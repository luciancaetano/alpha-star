#include "devtools/time_control.hpp"

namespace alpha::devtools {

void TimeControl::toggle_pause() {
  paused_ = !paused_;
  step_requested_ = false;
}

void TimeControl::request_step() { step_requested_ = paused_; }

int TimeControl::steps(int wanted) {
  if (!paused_) return wanted;
  const int granted = step_requested_ ? 1 : 0;
  step_requested_ = false;
  return granted;
}

}  // namespace alpha::devtools
