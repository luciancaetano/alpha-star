#pragma once

#include <cstdint>
#include <vector>

#include "core/input.hpp"

namespace alpha::core {

// Fingers on a touch screen between fixed steps, by id, in game pixels.
// take() hands one step its fingers (core::Finger: began / ended during the
// step, so a tap shorter than a step still arrives) and forgets lifted ones.
// At most kMaxTouches fingers: more are ignored until one lifts.
class Touches {
 public:
  // A finger already down just moves.
  void down(std::uint64_t finger, Vec2 pos);
  void move(std::uint64_t finger, Vec2 pos);
  void up(std::uint64_t finger);  // also a cancelled touch
  std::vector<Finger> take();

 private:
  Finger* find(std::uint64_t finger);
  std::vector<Finger> fingers_;
};

}  // namespace alpha::core
