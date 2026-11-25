#pragma once

#include <SDL3/SDL_events.h>

#include <optional>

#include "core/app_lifecycle.hpp"

namespace alpha::platform {

// The app lifecycle meaning of an SDL event (OS: background, foreground,
// terminating, low memory; window: focus, minimized, restored); nothing for
// every other event. Will- and did- events map to the same AppEvent (the
// state machine acts on the first).
std::optional<core::AppEvent> app_event(const SDL_Event& event);

}  // namespace alpha::platform
