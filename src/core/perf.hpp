#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <deque>
#include <string>
#include <string_view>

namespace alpha::core {

// Where a frame's time goes. Script = the game's own JS (update, handlers,
// ctx calls) without the natives timed separately below.
enum class Section : std::size_t { script, timers, physics, particles, animation, ui, render, audio };
inline constexpr std::size_t kSections = 8;
inline constexpr std::array<std::string_view, kSections> kSectionNames{
    "script", "timers", "physics", "particles", "animation", "ui", "render", "audio"};

// Measuring costs a clock read per section: only builds with a profiler
// reader (dev tools, MCP, debug tools / bench) pay it; shipping times nothing.
inline constexpr bool kProfile = ALPHA_DEVTOOLS || ALPHA_MCP || ALPHA_DEBUG_TOOLS;

// Milliseconds per section in one frame (all fixed steps of the frame summed).
struct FrameTimes {
  std::array<double, kSections> ms{};
  double& operator[](Section s) { return ms[static_cast<std::size_t>(s)]; }
  double operator[](Section s) const { return ms[static_cast<std::size_t>(s)]; }
  double total() const;
};

// Adds the scope's wall time to `into` (milliseconds); nothing without kProfile.
class ScopedTime {
 public:
  explicit ScopedTime(double& into);
  ~ScopedTime();
  ScopedTime(const ScopedTime&) = delete;
  ScopedTime& operator=(const ScopedTime&) = delete;

 private:
  double& into_;
  std::chrono::steady_clock::time_point start_;
};

// Runs f(), adding its wall time to `into`.
template <typename F>
void timed(double& into, F&& f) {
  const ScopedTime t(into);
  f();
}

struct Spread {
  double p50 = 0.0;
  double p95 = 0.0;
  double max = 0.0;
};

// p50 / p95 / max of `values` (nearest rank); zeros when empty.
Spread spread_of(std::deque<double> values);

// The newest `capacity` frames: per-section spreads and a graph's history.
class PerfHistory {
 public:
  explicit PerfHistory(std::size_t capacity);
  void push(const FrameTimes& frame);
  std::size_t size() const { return frames_.size(); }
  const FrameTimes& at(std::size_t i) const { return frames_.at(i); }
  // One section over the kept frames, or the frames' totals.
  Spread spread(Section s) const;
  Spread total() const;

 private:
  std::size_t capacity_;
  std::deque<FrameTimes> frames_;
};

// {"frames":n,"total":{"p50":..,"p95":..,"max":..},"script":{...},...} in ms
// (MCP game_state.perf, --perf bench reports).
std::string to_json(const PerfHistory& history);

}  // namespace alpha::core
