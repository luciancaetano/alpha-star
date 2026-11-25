#pragma once

#include <SDL3/SDL_events.h>

#include <atomic>
#include <mutex>
#include <optional>
#include <vector>

#include "core/app_lifecycle.hpp"

namespace alpha::platform {

// The app lifecycle meaning of an SDL event (OS: background, foreground,
// terminating, low memory; window: focus, minimized, restored); nothing for
// every other event. Will- and did- events map to the same AppEvent (the
// state machine acts on the first).
std::optional<core::AppEvent> app_event(const SDL_Event& event);

// The OS lifecycle events (background, foreground, terminating, low memory):
// SDL 3.2 never queues them, it only calls event watchers, on the pushing
// thread. The watch keeps them (under a lock) for the main loop to take();
// it only records, sets leaving() on background / termination and flushes
// stderr (saves and settings are already written atomically when set); it
// never touches JS. The loop stops stepping while leaving() and drives the
// lifecycle with what take() returns (which clears leaving()).
class LifecycleWatch {
 public:
  LifecycleWatch();
  ~LifecycleWatch();
  LifecycleWatch(const LifecycleWatch&) = delete;
  LifecycleWatch& operator=(const LifecycleWatch&) = delete;
  bool leaving() const { return leaving_.load(); }
  // The OS events since the last call, oldest first.
  std::vector<core::AppEvent> take();

 private:
  static bool SDLCALL on_event(void* self, SDL_Event* event);
  std::atomic<bool> leaving_{false};
  std::mutex lock_;
  std::vector<core::AppEvent> pending_;
};

}  // namespace alpha::platform
