#include "devtools/shake_toggle.hpp"

#include <cmath>

namespace alpha::devtools {

namespace {

float length(float x, float y, float z) { return std::sqrt(x * x + y * y + z * z); }

}  // namespace

// First sample, or the first after the sensor went silent (app paused): restart from rest.
bool ShakeToggle::restart_if_gap(Vec a, std::uint64_t ms) {
  const bool gap = !started_ || ms - last_ms_ > kGapMs;
  if (!gap) return false;
  last_ms_ = ms;
  started_ = true;
  gravity_ = a;
  above_ = false;
  reset_count();
  quiet_until_ms_ = ms + kSettleMs;
  return true;
}

void ShakeToggle::reset_count() {
  reversals_ = 0;
  last_jolt_ = {};
}

// A new jolt: counts when it reverses the previous one; true when it completes the gesture.
bool ShakeToggle::jolt(Vec linear, std::uint64_t ms) {
  if (ms < quiet_until_ms_) return false;
  const float dot = linear.x * last_jolt_.x + linear.y * last_jolt_.y + linear.z * last_jolt_.z;
  last_jolt_ = linear;
  if (dot >= 0) return false;
  if (reversals_ == 0 || ms - first_reversal_ms_ > kWindowMs) {
    reversals_ = 0;
    first_reversal_ms_ = ms;
  }
  if (++reversals_ < kReversals) return false;
  reset_count();
  quiet_until_ms_ = ms + kCooldownMs;
  return true;
}

bool ShakeToggle::sample(float x, float y, float z, std::uint64_t ms) {
  if (restart_if_gap({x, y, z}, ms)) return false;
  const float follow = std::fmin(1.0f, static_cast<float>(ms - last_ms_) / kGravityMs);
  last_ms_ = ms;
  const Vec linear{x - gravity_.x, y - gravity_.y, z - gravity_.z};
  const bool above = length(linear.x, linear.y, linear.z) > kJolt;
  const bool rising = above && !above_;
  above_ = above;
  gravity_.x += (x - gravity_.x) * follow;
  gravity_.y += (y - gravity_.y) * follow;
  gravity_.z += (z - gravity_.z) * follow;
  return rising && jolt(linear, ms);
}

}  // namespace alpha::devtools
