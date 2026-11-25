#include "core/touch.hpp"

#include <algorithm>

namespace alpha::core {

void Touches::down(std::uint64_t finger, float x) {
  up(finger);
  fingers_.push_back({finger, x});
}

void Touches::move(std::uint64_t finger, float x) {
  for (Finger& f : fingers_) {
    if (f.id == finger) f.x = x;
  }
}

void Touches::up(std::uint64_t finger) {
  std::erase_if(fingers_, [finger](const Finger& f) { return f.id == finger; });
}

InputSet Touches::held() const {
  InputSet held;
  if (fingers_.empty()) return held;
  held.set(*parse_input("Touch"));
  const bool left = std::any_of(fingers_.begin(), fingers_.end(), [](const Finger& f) { return f.x < 0.5f; });
  const bool right = std::any_of(fingers_.begin(), fingers_.end(), [](const Finger& f) { return f.x >= 0.5f; });
  held.set(*parse_input("TouchLeft"), left);
  held.set(*parse_input("TouchRight"), right);
  return held;
}

}  // namespace alpha::core
