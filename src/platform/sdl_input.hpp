#pragma once

#include <SDL3/SDL.h>

#include <optional>

#include "core/input.hpp"

namespace alpha::platform {

// Device state between fixed steps. A press shorter than one step still
// reaches the next step through `pressed`.
struct InputState {
  core::InputSet down;
  core::InputSet pressed;
};

// Maps SDL devices to core input codes; nullopt for unmapped buttons/keys.
std::optional<core::InputCode> from_scancode(SDL_Scancode code);
std::optional<core::InputCode> from_mouse(Uint8 button);
std::optional<core::InputCode> from_gamepad(Uint8 button);

// Updates state from one SDL event; ignores unrelated events and key repeat.
void apply_event(InputState& state, const SDL_Event& event);
// Inputs to feed this step (held or pressed since last step); clears presses.
core::InputSet consume(InputState& state);

}  // namespace alpha::platform
