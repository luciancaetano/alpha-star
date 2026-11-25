#pragma once

#include <cstdint>
#include <vector>

#include "core/input.hpp"

namespace alpha::core {

// Fingers on a touch screen as digital inputs of player 1 (virtual buttons):
// "Touch" while any finger is down, "TouchLeft" / "TouchRight" while one is on
// that half of the window. The pointer position comes from the mouse SDL
// emulates for the first finger (ctx.input.mouse(), "MouseLeft").
class Touches {
 public:
  // x: 0..1 across the window. A finger already down just moves.
  void down(std::uint64_t finger, float x);
  void move(std::uint64_t finger, float x);
  void up(std::uint64_t finger);  // also a cancelled touch
  InputSet held() const;

 private:
  struct Finger {
    std::uint64_t id;
    float x;
  };
  std::vector<Finger> fingers_;
};

}  // namespace alpha::core
