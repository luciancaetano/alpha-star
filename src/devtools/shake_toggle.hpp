#pragma once

#include <cstdint>

namespace alpha::devtools {

// The dev tools' phone toggle, as in React Native: a deliberate shake. Pure (no SDL):
// accelerometer samples in m/s^2 (gravity included), times in milliseconds.
// Gravity is tracked by a low-pass filter; a jolt is the linear acceleration (sample minus
// gravity) rising above kJolt. Jolts whose direction reverses the previous jolt's count as a
// reversal; kReversals of them within kWindowMs toggle. After a toggle nothing counts for
// kCooldownMs; after a sensor gap of kGapMs (pause/resume) nothing counts for kSettleMs.
// Taps, tilts and walking stay below kJolt or never reverse fast enough.
class ShakeToggle {
 public:
  static constexpr float kJolt = 22.0f;  // ~2.2 g of linear acceleration
  static constexpr int kReversals = 3;   // back-forth-back-forth
  static constexpr std::uint64_t kWindowMs = 800;
  static constexpr std::uint64_t kCooldownMs = 1500;
  static constexpr std::uint64_t kGapMs = 500;
  static constexpr std::uint64_t kSettleMs = 500;
  static constexpr float kGravityMs = 300.0f;  // low-pass time constant of the gravity estimate

  // True when this sample completes a shake gesture.
  bool sample(float x, float y, float z, std::uint64_t ms);

 private:
  struct Vec {
    float x = 0, y = 0, z = 0;
  };
  bool restart_if_gap(Vec a, std::uint64_t ms);
  bool jolt(Vec linear, std::uint64_t ms);
  void reset_count();

  Vec gravity_;
  Vec last_jolt_;
  bool started_ = false;
  bool above_ = false;
  int reversals_ = 0;
  std::uint64_t last_ms_ = 0;
  std::uint64_t first_reversal_ms_ = 0;
  std::uint64_t quiet_until_ms_ = 0;  // no jolt counts before this time
};

}  // namespace alpha::devtools
