#pragma once

namespace alpha::devtools {

// Dev-tools pause/step over the fixed-step clock. Running: the clock decides.
// Paused: nothing runs, except exactly one step after each step request.
class TimeControl {
 public:
  void toggle_pause();
  // Ignored while running.
  void request_step();
  bool paused() const { return paused_; }
  // Steps to run this frame, given what the clock wants.
  int steps(int wanted);

 private:
  bool paused_ = false;
  bool step_requested_ = false;
};

}  // namespace alpha::devtools
