#include "script/app_events.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>

#include <cstdio>
#include <utility>

#include "platform/lifecycle.hpp"

namespace alpha::script {

AppEvents::AppEvents(bool interactive) : interactive_(interactive), machine_(interactive) {}

bool AppEvents::handle(const SDL_Event& event, Host& host) {
  const auto app = platform::app_event(event);
  if (!app) return false;
  watch_.seen(*app);
  apply(machine_.handle(*app), host);
  return true;
}

void AppEvents::configure(Host& host) {
  apply(machine_.pause_on_focus_loss(interactive_ && host.info().pause_on_focus_loss), host);
}

int AppEvents::steps(core::FixedStep& clock, int wanted) {
  if (std::exchange(resumed_, false)) clock.reset();
  return paused() ? 0 : wanted;
}

void AppEvents::idle() const {
  if (paused()) SDL_WaitEventTimeout(nullptr, 50);
}

// Saves and settings are written (atomically) when the game sets them, so
// nothing of theirs is pending; the log is flushed. onPause runs first, so a
// game saving there is on disk before the OS may suspend or kill the app.
void AppEvents::apply(const core::AppEffects& effects, Host& host) {
  if (effects.pause) host.set_app_paused(true);
  if (effects.resume) host.set_app_paused(false);
  resumed_ = resumed_ || effects.resume;
  if (effects.low_memory) {
    SDL_Log("the system is low on memory");
    host.low_memory();
  }
  if (effects.flush) std::fflush(stderr);
}

}  // namespace alpha::script
