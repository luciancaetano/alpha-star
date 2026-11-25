#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <optional>

#include "core/input.hpp"
#include "core/pad_slots.hpp"

namespace alpha::platform {

// Device state between fixed steps, per player. A press shorter than one
// step still reaches the next step through `pressed`. Analog values are raw
// (deadzones belong to the game's core::Axes).
struct InputState {
  std::array<core::InputSet, core::kMaxPlayers> down;
  std::array<core::InputSet, core::kMaxPlayers> pressed;
  std::array<core::AnalogValues, core::kMaxPlayers> analog{};
  core::Vec2 mouse{};  // from the last motion event (already in game pixels)
};

// Maps SDL devices to core input codes; nullopt for unmapped buttons/keys.
std::optional<core::InputCode> from_scancode(SDL_Scancode code);
std::optional<core::InputCode> from_mouse(Uint8 button);
std::optional<core::InputCode> from_gamepad(Uint8 button);

// Updates state from one SDL event; ignores unrelated events, key repeat and
// gamepads without a slot. Keyboard and mouse are player 1; a gamepad is the
// player of its slot.
void apply_event(InputState& state, const SDL_Event& event, const core::PadSlots& slots);
// A player's device went away: nothing of theirs stays held.
void release_player(InputState& state, std::size_t player);
// Inputs to feed this step (held or pressed since last step); clears presses.
core::InputFrame consume(InputState& state, const core::PadSlots& slots);

}  // namespace alpha::platform
