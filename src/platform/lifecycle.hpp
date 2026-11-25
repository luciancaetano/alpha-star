#pragma once

#include <SDL3/SDL_events.h>

#include <atomic>
#include <optional>

#include "core/app_lifecycle.hpp"

namespace alpha::platform {

// The app lifecycle meaning of an SDL event (OS: background, foreground,
// terminating, low memory; window: focus, minimized, restored); nothing for
// every other event. Will- and did- events map to the same AppEvent (the
// state machine acts on the first).
std::optional<core::AppEvent> app_event(const SDL_Event& event);

// SDL_AddEventWatch for the moment the OS announces background / termination
// (SDL docs: on mobile the app may be suspended or killed before the main loop
// polls the queued event). The callback runs on the pushing thread: it only
// sets an atomic flag and flushes stderr (saves and settings are already
// written atomically when set); it never touches JS. The main loop stops
// stepping while leaving() and the queued event, once polled, drives the
// lifecycle as usual (seen() clears the flag).
class LifecycleWatch {
 public:
  LifecycleWatch();
  ~LifecycleWatch();
  LifecycleWatch(const LifecycleWatch&) = delete;
  LifecycleWatch& operator=(const LifecycleWatch&) = delete;
  bool leaving() const { return leaving_.load(); }
  void seen(core::AppEvent event);

 private:
  static bool SDLCALL on_event(void* self, SDL_Event* event);
  std::atomic<bool> leaving_{false};
};

}  // namespace alpha::platform
