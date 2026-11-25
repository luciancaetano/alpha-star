#include "core/controls.hpp"

#include <string>

#include "core/error.hpp"

namespace alpha::core {

void Controls::update(const InputFrame& frame) {
  actions.update(frame);
  axes.update(frame);
  players = frame.connected;
  mouse = frame.mouse;
}

void Controls::request_rumble(float strength, float seconds, std::size_t player) {
  if (!(strength >= 0.0f && strength <= 1.0f)) {
    throw Error("rumble strength must be 0..1, got " + std::to_string(strength));
  }
  if (!(seconds >= 0.0f && seconds <= kMaxRumbleSeconds)) {
    throw Error("rumble seconds must be 0..10, got " + std::to_string(seconds));
  }
  for (std::size_t p = 0; p < kMaxPlayers; ++p) {
    if (player == kAnyPlayer || player == p) rumble_[p] = Rumble{strength, seconds};
  }
}

RumbleRequests Controls::take_rumble() {
  RumbleRequests out = rumble_;
  rumble_ = {};
  return out;
}

}  // namespace alpha::core
