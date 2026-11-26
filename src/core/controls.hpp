#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include "core/axes.hpp"
#include "core/input.hpp"
#include "core/touch_pad.hpp"

namespace alpha::core {

// A gamepad vibration: strength 0..1 for `seconds` (0 stops it).
struct Rumble {
  float strength = 0.0f;
  float seconds = 0.0f;
};
inline constexpr float kMaxRumbleSeconds = 10.0f;
using RumbleRequests = std::array<std::optional<Rumble>, kMaxPlayers>;

// A game's view of its players' devices: actions, axes, touch controls and
// how many players have a device. Updated once per fixed step: the touch pad
// captures fingers first; its inputs and stick axes then feed actions and axes.
class Controls {
 public:
  Actions actions;
  Axes axes;
  TouchPad touch;
  std::size_t players = 1;
  Vec2 mouse{};  // game pixels, as of this step (a free finger's, while one touches)
  std::vector<Finger> fingers;  // this step's fingers no touch control captured

  void update(const InputFrame& frame);
  // player 0..kMaxPlayers-1 or kAnyPlayer (all). Throws Error unless
  // 0 <= strength <= 1 and 0 <= seconds <= kMaxRumbleSeconds.
  void request_rumble(float strength, float seconds, std::size_t player);
  // Requests since the last call (the latest per player), cleared.
  RumbleRequests take_rumble();

 private:
  RumbleRequests rumble_{};
};

}  // namespace alpha::core
