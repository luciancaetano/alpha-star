#pragma once

#include <cstdint>
#include <vector>

namespace alpha::core {

using TimerId = std::uint32_t;

// Game-time timers. Deterministic: driven only by advance(dt).
class Timers {
 public:
  static constexpr double kMinSeconds = 0.001;
  // Fires every `period` seconds, first after one period. Throws Error if period < kMinSeconds (or NaN).
  TimerId every(double period);
  // Fires once after `delay` seconds. Throws Error if delay < kMinSeconds (or NaN).
  TimerId after(double delay);
  void cancel(TimerId id);
  void clear();
  // Advances time; returns fired timers ordered by due time, then creation.
  std::vector<TimerId> advance(double dt);

 private:
  struct Timer {
    TimerId id;
    double due;
    double period;  // 0 = one-shot
  };
  TimerId add(double delay, double period);
  Timer* next_due();

  std::vector<Timer> timers_;
  double now_ = 0.0;
  TimerId next_id_ = 1;
};

}  // namespace alpha::core
