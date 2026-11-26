#include "core/touch.hpp"

#include <algorithm>

#include "core/limits.hpp"

namespace alpha::core {

Finger* Touches::find(std::uint64_t finger) {
  const auto it = std::find_if(fingers_.begin(), fingers_.end(), [finger](const Finger& f) { return f.id == finger; });
  return it == fingers_.end() ? nullptr : &*it;
}

void Touches::down(std::uint64_t finger, Vec2 pos) {
  if (Finger* f = find(finger); f && !f->ended) return move(finger, pos);
  if (fingers_.size() >= kMaxTouches) return;
  fingers_.push_back({finger, pos, true, false});
}

void Touches::move(std::uint64_t finger, Vec2 pos) {
  if (Finger* f = find(finger); f && !f->ended) f->pos = pos;
}

void Touches::up(std::uint64_t finger) {
  if (Finger* f = find(finger)) f->ended = true;
}

std::vector<Finger> Touches::take() {
  std::vector<Finger> out = fingers_;
  std::erase_if(fingers_, [](const Finger& f) { return f.ended; });
  for (Finger& f : fingers_) f.began = false;
  return out;
}

}  // namespace alpha::core
