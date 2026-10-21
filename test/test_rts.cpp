// Checks lib/rts against Legion2/Somfy_Remote_Lib, compiled here with a
// recording Arduino stub. Run with `make test`.
#include <stdio.h>
#include <random>
#include <vector>

#include "SomfyRemote.h"
#include "rts.h"

namespace {

int failures = 0;

void check(bool ok, const char* what, uint32_t address, uint16_t code, int command) {
  if (ok) return;
  failures++;
  if (failures <= 10) {
    printf("FAIL %s: address %06x code %u command %x\n", what, address, code, command);
  }
}

class FixedCode : public RollingCodeStorage {
 public:
  uint16_t nextCode() override { return 0; }
};

const rts::Command kCommands[] = {
    rts::Command::My,   rts::Command::Up,     rts::Command::MyUp,
    rts::Command::Down, rts::Command::MyDown, rts::Command::UpDown,
    rts::Command::Prog, rts::Command::SunFlag, rts::Command::Flag,
};

void compareOne(uint32_t address, uint16_t code, rts::Command command, int repeats) {
  FixedCode storage;
  SomfyRemote reference(0, address, &storage);
  stub::pulses().clear();
  stub::level() = LOW;
  reference.sendCommandWithCode(static_cast<Command>(command), code, repeats);

  uint8_t frame[rts::kFrameLen];
  rts::buildFrame(frame, command, code, address);
  std::vector<rts::Pulse> ours(rts::maxPulses(repeats));
  size_t n = rts::encode(frame, repeats, ours.data(), ours.size());

  const auto& theirs = stub::pulses();
  bool same = n == theirs.size();
  for (size_t i = 0; same && i < n; i++) {
    same = ours[i].high == theirs[i].high && ours[i].us == theirs[i].us;
  }
  check(n > 0, "encode overflowed maxPulses", address, code, (int)command);
  check(same, "waveform differs from reference", address, code, (int)command);

  rts::Command gotCommand;
  uint16_t gotCode;
  uint32_t gotAddress;
  bool ok = rts::decodeFrame(frame, &gotCommand, &gotCode, &gotAddress);
  check(ok && gotCommand == command && gotCode == code && gotAddress == address,
        "decode round trip", address, code, (int)command);

  // A flipped bit in the last byte must fail the checksum. (In bytes 1-5 it
  // also flips the same bit of the next de-obfuscated byte and cancels out,
  // a blind spot of the Somfy checksum itself.)
  frame[6] ^= 0x10;
  check(!rts::decodeFrame(frame, &gotCommand, &gotCode, &gotAddress),
        "corrupt frame passed checksum", address, code, (int)command);
}

}  // namespace

int main() {
  int cases = 0;
  // Sample addresses (not real paired ones), every command, edge rolling codes.
  for (uint32_t address : {0x123456u, 0xABCDEFu, 0x0F0F0Fu, 0u, 0xFFFFFFu}) {
    for (auto command : kCommands) {
      for (uint16_t code : {0, 1, 2, 255, 256, 0x7FFF, 0xFFFF}) {
        for (int repeats : {0, 4, 8}) {
          compareOne(address, code, command, repeats);
          cases++;
        }
      }
    }
  }
  std::mt19937 rng(20261020);
  for (int i = 0; i < 20000; i++) {
    compareOne(rng() & 0xFFFFFF, rng() & 0xFFFF, kCommands[rng() % 9], rng() % 10);
    cases++;
  }
  printf("%d cases, %d failures\n", cases, failures);
  return failures == 0 ? 0 : 1;
}
