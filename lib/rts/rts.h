// Somfy RTS frame building and pulse encoding.
//
// Pure C++, no Arduino: the same code runs on the ESP32 and in the native
// tests on a laptop. Protocol details follow the pushstack write-up and
// Legion2/Somfy_Remote_Lib (Apache 2.0), which the tests check against.
#pragma once

#include <stdint.h>
#include <stddef.h>

namespace rts {

enum class Command : uint8_t {
  My = 0x1,
  Up = 0x2,
  MyUp = 0x3,
  Down = 0x4,
  MyDown = 0x5,
  UpDown = 0x6,
  Prog = 0x8,
  SunFlag = 0x9,
  Flag = 0xA,
};

constexpr size_t kFrameLen = 7;

// Fills `frame` with the 7 obfuscated bytes that go on air.
// `address` is the 24-bit remote address, `code` the rolling code.
void buildFrame(uint8_t frame[kFrameLen], Command command, uint16_t code, uint32_t address);

// One stretch of the transmitter held high (carrier on) or low (off).
struct Pulse {
  bool high;
  uint32_t us;
};

// Upper bound on pulses from encode(): wake-up + gap, and per frame up to
// 7 hardware syncs, a software sync, 112 data half-symbols and the trailer.
constexpr size_t maxPulses(int repeats) {
  return 2 + (size_t)(repeats + 1) * (2 * 7 + 2 + 112 + 1);
}

// Encodes the full transmission for one button press: the first frame with
// the wake-up pulse and 2 hardware syncs, then `repeats` frames with 7 syncs,
// each followed by its inter-frame gap. Adjacent pulses of the same level are
// merged. Returns the number of pulses written, or 0 if `cap` is too small.
size_t encode(const uint8_t frame[kFrameLen], int repeats, Pulse* out, size_t cap);

// Undoes the obfuscation and checks the checksum. Returns false on a bad
// checksum.
bool decodeFrame(const uint8_t frame[kFrameLen], Command* command, uint16_t* code, uint32_t* address);

}  // namespace rts
