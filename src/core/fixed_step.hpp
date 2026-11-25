#pragma once

namespace alpha::core {

// Converts variable real time into a whole number of fixed simulation steps.
class FixedStep {
 public:
  FixedStep(double dt, int max_steps);

  // Adds real elapsed seconds; returns how many steps to run now. After a
  // stall the count is capped at max_steps and the backlog is dropped.
  int advance(double elapsed);
  double dt() const { return dt_; }
  // How far into the next step real time is, in [0, 1): the render blend
  // factor between the previous and the current simulation state.
  double fraction() const { return accumulated_ / dt_; }
  // Forget accumulated time (the app resumes: no catch-up burst of steps).
  void reset() { accumulated_ = 0.0; }

 private:
  double dt_;
  int max_steps_;
  double accumulated_ = 0.0;
};

}  // namespace alpha::core
