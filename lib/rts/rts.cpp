#include "rts.h"

namespace rts {

namespace {

constexpr uint32_t kSymbol = 640;      // half a Manchester bit, in us
constexpr uint32_t kWakeHigh = 9415;
constexpr uint32_t kWakeLow = 9565;
constexpr uint32_t kWakeGap = 80000;   // silence after the wake-up pulse
constexpr uint32_t kSoftSync = 4550;
constexpr uint32_t kTrailer = 415;
constexpr uint32_t kFrameGap = 30000;  // silence after every frame

uint8_t checksum(const uint8_t frame[kFrameLen]) {
  uint8_t sum = 0;
  for (size_t i = 0; i < kFrameLen; i++) sum ^= frame[i] ^ (frame[i] >> 4);
  return sum & 0x0F;
}

class Writer {
 public:
  Writer(Pulse* out, size_t cap) : out_(out), cap_(cap) {}

  void put(bool high, uint32_t us) {
    if (n_ > 0 && out_[n_ - 1].high == high) {
      out_[n_ - 1].us += us;
      return;
    }
    if (n_ == cap_) {
      overflow_ = true;
      return;
    }
    out_[n_++] = {high, us};
  }

  size_t result() const { return overflow_ ? 0 : n_; }

 private:
  Pulse* out_;
  size_t cap_;
  size_t n_ = 0;
  bool overflow_ = false;
};

void encodeFrame(Writer& w, const uint8_t frame[kFrameLen], int syncs) {
  for (int i = 0; i < syncs; i++) {
    w.put(true, 4 * kSymbol);
    w.put(false, 4 * kSymbol);
  }
  w.put(true, kSoftSync);
  w.put(false, kSymbol);
  // MSB first; a 1 is a rising edge mid-bit, a 0 a falling one.
  for (size_t i = 0; i < kFrameLen * 8; i++) {
    bool bit = (frame[i / 8] >> (7 - i % 8)) & 1;
    w.put(!bit, kSymbol);
    w.put(bit, kSymbol);
  }
  w.put(false, kTrailer);
  w.put(false, kFrameGap);
}

}  // namespace

void buildFrame(uint8_t frame[kFrameLen], Command command, uint16_t code, uint32_t address) {
  frame[0] = 0xA7;  // key byte; receivers don't check it
  frame[1] = static_cast<uint8_t>(command) << 4;
  frame[2] = code >> 8;
  frame[3] = code;
  frame[4] = address >> 16;
  frame[5] = address >> 8;
  frame[6] = address;
  frame[1] |= checksum(frame);
  for (size_t i = 1; i < kFrameLen; i++) frame[i] ^= frame[i - 1];
}

size_t encode(const uint8_t frame[kFrameLen], int repeats, Pulse* out, size_t cap) {
  Writer w(out, cap);
  w.put(true, kWakeHigh);
  w.put(false, kWakeLow);
  w.put(false, kWakeGap);
  encodeFrame(w, frame, 2);
  for (int i = 0; i < repeats; i++) encodeFrame(w, frame, 7);
  return w.result();
}

bool decodeFrame(const uint8_t frame[kFrameLen], Command* command, uint16_t* code, uint32_t* address) {
  uint8_t plain[kFrameLen];
  plain[0] = frame[0];
  for (size_t i = 1; i < kFrameLen; i++) plain[i] = frame[i] ^ frame[i - 1];
  // The checksum was computed with its own nibble zero; folding the real
  // nibble back in cancels it, so a good frame checksums to 0.
  if (checksum(plain) != 0) return false;
  *command = static_cast<Command>(plain[1] >> 4);
  *code = (uint16_t)(plain[2] << 8 | plain[3]);
  *address = (uint32_t)plain[4] << 16 | (uint32_t)plain[5] << 8 | plain[6];
  return true;
}

}  // namespace rts
