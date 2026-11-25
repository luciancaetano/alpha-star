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

 private:
  double dt_;
  int max_steps_;
  double accumulated_ = 0.0;
};

}  // namespace alpha::core
