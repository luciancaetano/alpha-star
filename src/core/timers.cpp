#include "core/timers.hpp"

#include <algorithm>
#include <string>

#include "core/error.hpp"

namespace alpha::core {

namespace {
constexpr double kEpsilon = 1e-9;  // absorbs float drift from summed dt
}

TimerId Timers::every(double period) { return add(period, period); }

TimerId Timers::after(double delay) { return add(delay, 0.0); }

TimerId Timers::add(double delay, double period) {
  // A shorter period would fire thousands of times per step (one advance
  // collects every firing): 1 ms is far below any frame.
  if (!(delay >= kMinSeconds)) throw Error("timer seconds must be >= 0.001, got " + std::to_string(delay));
  timers_.push_back({next_id_, now_ + delay, period});
  return next_id_++;
}

void Timers::cancel(TimerId id) {
  std::erase_if(timers_, [id](const Timer& t) { return t.id == id; });
}

void Timers::clear() { timers_.clear(); }

Timers::Timer* Timers::next_due() {
  Timer* best = nullptr;
  for (Timer& t : timers_) {
    if (t.due > now_ + kEpsilon) continue;
    if (!best || t.due < best->due || (t.due == best->due && t.id < best->id)) best = &t;
  }
  return best;
}

std::vector<TimerId> Timers::advance(double dt) {
  now_ += dt;
  std::vector<TimerId> fired;
  while (Timer* t = next_due()) {
    fired.push_back(t->id);
    if (t->period > 0.0) {
      t->due += t->period;
    } else {
      cancel(t->id);
    }
  }
  return fired;
}

}  // namespace alpha::core
