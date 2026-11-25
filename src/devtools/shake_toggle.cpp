#include "devtools/shake_toggle.hpp"

#include <cmath>

namespace alpha::devtools {

bool ShakeToggle::sample(float x, float y, float z, std::uint64_t ms) {
  const bool above = std::sqrt(x * x + y * y + z * z) > kThreshold;
  const bool rising = above && !above_;
  above_ = above;
  if (!rising) return false;
  while (!shakes_.empty() && ms - shakes_.front() > kWindowMs) shakes_.pop_front();
  shakes_.push_back(ms);
  if (shakes_.size() < static_cast<std::size_t>(kShakes)) return false;
  shakes_.clear();
  return true;
}

}  // namespace alpha::devtools
