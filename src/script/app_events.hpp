#pragma once

#include <SDL3/SDL_events.h>

#include "core/app_lifecycle.hpp"
#include "core/fixed_step.hpp"
#include "platform/lifecycle.hpp"
#include "script/host.hpp"

namespace alpha::script {

// The main loop's app lifecycle: SDL events (platform::app_event) drive the
// core::AppLifecycle machine and its effects reach the game: ctx.onPause /
// onResume / onLowMemory hooks, ctx.app.paused, the log flushed. The loop
// takes its steps through steps() and pauses audio while paused().
class AppEvents {
 public:
  // interactive = false (agent control, --frames): focus and minimizing never
  // pause (the agent owns time); the OS background still does.
  explicit AppEvents(bool interactive);
  // True when `event` was a lifecycle event (handled here).
  bool handle(const SDL_Event& event, Host& host);
  // The OS events SDL gave only to the watch (background, foreground,
  // terminating, low memory): once per frame, after polling.
  void drain(Host& host);
  // Applies config.pauseOnFocusLoss of the running game (it may reload).
  void configure(Host& host);
  // Also while the OS has announced background / termination not yet polled.
  bool paused() const { return machine_.paused() || watch_.leaving(); }
  // The steps to run this frame: none while paused; on resume the clock
  // restarts (no catch-up burst of the paused time).
  int steps(core::FixedStep& clock, int wanted);
  // While paused, waits up to ~50 ms for an event (no busy loop when minimized).
  void idle() const;

 private:
  void apply(const core::AppEffects& effects, Host& host);

  bool interactive_;
  core::AppLifecycle machine_;
  bool resumed_ = false;
  platform::LifecycleWatch watch_;
};

}  // namespace alpha::script
