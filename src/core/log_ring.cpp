#include "core/log_ring.hpp"

#include <array>

namespace alpha::core {

std::string_view level_name(LogLevel level) {
  static constexpr std::array<std::string_view, 3> kNames{"log", "warn", "error"};
  return kNames.at(static_cast<std::size_t>(level));
}

LogRing::LogRing(std::size_t capacity) : capacity_(capacity) {}

void LogRing::push(LogLevel level, std::string text) {
  lines_.push_back({level, std::move(text)});
  if (lines_.size() > capacity_) lines_.pop_front();
  ++total_;
}

void LogRing::clear() { lines_.clear(); }

}  // namespace alpha::core
