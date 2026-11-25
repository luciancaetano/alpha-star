#pragma once

#include <cstdint>
#include <deque>

namespace alpha::devtools {

// The dev tools' phone toggle, as in React Native: shake the device. Pure (no SDL):
// accelerometer samples in m/s^2 (gravity included), times in milliseconds. A shake is
// the acceleration rising above kThreshold; kShakes of them within kWindowMs toggle.
class ShakeToggle {
 public:
  static constexpr float kThreshold = 13.0f;  // ~1.33 g, React Native's ShakeDetector
  static constexpr int kShakes = 4;
  static constexpr std::uint64_t kWindowMs = 1000;

  // True when this sample completes a shake gesture.
  bool sample(float x, float y, float z, std::uint64_t ms);

 private:
  std::deque<std::uint64_t> shakes_;  // when each recent shake started
  bool above_ = false;
};

}  // namespace alpha::devtools
