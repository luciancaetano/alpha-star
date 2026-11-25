#include "core/app_lifecycle.hpp"

namespace alpha::core {

AppLifecycle::AppLifecycle(bool pause_on_focus_loss) : focus_rule_(pause_on_focus_loss) {}

bool AppLifecycle::paused() const {
  return background_ || terminating_ || (focus_rule_ && (minimized_ || unfocused_));
}

void AppLifecycle::record(AppEvent event) {
  switch (event) {
    case AppEvent::background: background_ = true; break;
    case AppEvent::foreground: background_ = false; break;
    case AppEvent::focus_lost: unfocused_ = true; break;
    case AppEvent::focus_gained: unfocused_ = false; break;
    case AppEvent::minimized: minimized_ = true; break;
    case AppEvent::restored: minimized_ = false; break;
    case AppEvent::terminating: terminating_ = true; break;
    case AppEvent::low_memory: break;
  }
}

AppEffects AppLifecycle::edge(bool was_paused) const {
  AppEffects out;
  out.pause = !was_paused && paused();
  out.resume = was_paused && !paused();
  return out;
}

AppEffects AppLifecycle::handle(AppEvent event) {
  const bool was_paused = paused();
  record(event);
  AppEffects out = edge(was_paused);
  out.flush = event == AppEvent::background || event == AppEvent::terminating;
  out.low_memory = event == AppEvent::low_memory;
  return out;
}

AppEffects AppLifecycle::pause_on_focus_loss(bool on) {
  const bool was_paused = paused();
  focus_rule_ = on;
  return edge(was_paused);
}

}  // namespace alpha::core
