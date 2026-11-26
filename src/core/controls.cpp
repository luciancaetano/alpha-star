#include "core/controls.hpp"

#include <string>

#include "core/error.hpp"

namespace alpha::core {

void Controls::update(const InputFrame& frame) {
  TouchFrame t = touch.update(frame.fingers);
  InputFrame in = frame;
  for (std::size_t p = 0; p < kMaxPlayers; ++p) {
    in.players[p].held |= t.held[p];
    in.players[p].touch = t.analog[p];
  }
  actions.update(in);
  axes.update(in);
  players = frame.connected;
  mouse = t.free.empty() ? frame.mouse : t.free.front().pos;
  fingers = std::move(t.free);
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
