#include "core/fixed_step.hpp"

namespace alpha::core {

FixedStep::FixedStep(double dt, int max_steps) : dt_(dt), max_steps_(max_steps) {}

int FixedStep::advance(double elapsed) {
  accumulated_ += elapsed;
  const int steps = static_cast<int>(accumulated_ / dt_);
  if (steps > max_steps_) {
    accumulated_ = 0.0;
    return max_steps_;
  }
  accumulated_ -= steps * dt_;
  return steps;
}

}  // namespace alpha::core
