#pragma once

namespace alpha::core {

// What the OS / window tells the app (platform/lifecycle maps SDL events here).
enum class AppEvent { background, foreground, focus_lost, focus_gained, minimized, restored, terminating, low_memory };

// What the engine must do now because of one event.
struct AppEffects {
  bool pause = false;       // stop the simulation and audio; run onPause hooks
  bool resume = false;      // run again from now (no catch-up); run onResume hooks
  bool flush = false;       // persist what must survive: the OS may kill the app next
  bool low_memory = false;  // log it; run onLowMemory hooks
};

// The app's pause state machine (pure). Paused while any reason holds: in the
// background, terminating, or - with pauseOnFocusLoss - minimized or without
// focus. pause / resume fire only on the edges, so repeated events (will +
// did enter background) act once.
class AppLifecycle {
 public:
  explicit AppLifecycle(bool pause_on_focus_loss);
  AppEffects handle(AppEvent event);
  // config.pauseOnFocusLoss changed (a reloaded game): applies at once.
  AppEffects pause_on_focus_loss(bool on);
  bool paused() const;

 private:
  void record(AppEvent event);
  AppEffects edge(bool was_paused) const;

  bool focus_rule_;
  bool background_ = false;
  bool terminating_ = false;
  bool minimized_ = false;
  bool unfocused_ = false;
};

}  // namespace alpha::core
