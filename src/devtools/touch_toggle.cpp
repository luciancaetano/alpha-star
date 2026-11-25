#include "devtools/touch_toggle.hpp"

#include <algorithm>
#include <cmath>

namespace alpha::devtools {

void TouchToggle::down(std::uint64_t finger, float x, float y, std::uint64_t ms) {
  fingers_.push_back({finger, x, y});
  if (fingers_.size() != kFingers) {
    armed_ = false;  // a fourth finger is not the gesture
    return;
  }
  since_ = ms;
  armed_ = true;
}

void TouchToggle::move(std::uint64_t finger, float x, float y) {
  for (const Finger& f : fingers_) {
    if (f.id == finger && (std::fabs(f.x - x) > kSlop || std::fabs(f.y - y) > kSlop)) armed_ = false;
  }
}

void TouchToggle::up(std::uint64_t finger) {
  std::erase_if(fingers_, [finger](const Finger& f) { return f.id == finger; });
  armed_ = false;
}

bool TouchToggle::poll(std::uint64_t ms) {
  if (!armed_ || ms - since_ < kHoldMs) return false;
  armed_ = false;
  return true;
}

}  // namespace alpha::devtools
