#include "core/perf.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <utility>

namespace alpha::core {

double FrameTimes::total() const { return std::accumulate(ms.begin(), ms.end(), 0.0); }

ScopedTime::ScopedTime(double& into) : into_(into) {
  if constexpr (kProfile) start_ = std::chrono::steady_clock::now();
}

ScopedTime::~ScopedTime() {
  if constexpr (kProfile) {
    into_ += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start_).count();
  }
}

namespace {

// Nearest rank: the smallest value with at least `pct` percent of values at or below it.
double rank(const std::deque<double>& sorted, double pct) {
  const auto n = static_cast<double>(sorted.size());
  const auto index = static_cast<std::size_t>(std::max(1.0, std::ceil(pct / 100.0 * n))) - 1;
  return sorted[index];
}

}  // namespace

Spread spread_of(std::deque<double> values) {
  if (values.empty()) return {};
  std::sort(values.begin(), values.end());
  return {rank(values, 50), rank(values, 95), values.back()};
}

PerfHistory::PerfHistory(std::size_t capacity) : capacity_(std::max<std::size_t>(capacity, 1)) {}

void PerfHistory::push(const FrameTimes& frame) {
  frames_.push_back(frame);
  if (frames_.size() > capacity_) frames_.pop_front();
}

Spread PerfHistory::spread(Section s) const {
  std::deque<double> values;
  for (const FrameTimes& f : frames_) values.push_back(f[s]);
  return spread_of(std::move(values));
}

Spread PerfHistory::total() const {
  std::deque<double> values;
  for (const FrameTimes& f : frames_) values.push_back(f.total());
  return spread_of(std::move(values));
}

namespace {

std::string spread_json(std::string_view name, const Spread& s) {
  std::array<char, 160> buf{};
  std::snprintf(buf.data(), buf.size(), ",\"%.*s\":{\"p50\":%.4f,\"p95\":%.4f,\"max\":%.4f}",
                static_cast<int>(name.size()), name.data(), s.p50, s.p95, s.max);
  return buf.data();
}

}  // namespace

std::string to_json(const PerfHistory& history) {
  std::string out = "{\"frames\":" + std::to_string(history.size()) + spread_json("total", history.total());
  for (std::size_t i = 0; i < kSections; ++i) out += spread_json(kSectionNames[i], history.spread(static_cast<Section>(i)));
  return out + "}";
}

}  // namespace alpha::core
