// Just enough of Arduino.h to compile Legion2/Somfy_Remote_Lib on a laptop.
// digitalWrite and the delays record the waveform instead of driving a pin.
#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <string>
#include <vector>

typedef uint8_t byte;

#define HIGH 1
#define LOW 0
#define OUTPUT 1

struct RecordedPulse {
  bool high;
  uint32_t us;
};

namespace stub {
inline std::vector<RecordedPulse>& pulses() {
  static std::vector<RecordedPulse> v;
  return v;
}
inline int& level() {
  static int l = LOW;
  return l;
}
inline void hold(uint32_t us) {
  auto& v = pulses();
  bool high = level() == HIGH;
  if (!v.empty() && v.back().high == high) {
    v.back().us += us;
  } else {
    v.push_back({high, us});
  }
}
}  // namespace stub

inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t, uint8_t value) { stub::level() = value; }
inline void delayMicroseconds(uint32_t us) { stub::hold(us); }
inline void delay(uint32_t ms) { stub::hold(ms * 1000); }
inline void noInterrupts() {}
inline void interrupts() {}

#define HEX 16

struct SerialStub {
  template <typename... T>
  void print(T...) {}
  template <typename... T>
  void println(T...) {}
};
inline SerialStub Serial;

class String : public std::string {
 public:
  using std::string::string;
  String(const std::string& s) : std::string(s) {}
  bool equalsIgnoreCase(const char* other) const { return strcasecmp(c_str(), other) == 0; }
};
