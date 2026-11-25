#pragma once

#include <cstddef>
#include <deque>
#include <string>
#include <string_view>

namespace alpha::core {

enum class LogLevel { Info, Warn, Error };

// "log", "warn" or "error": the console method that wrote the line.
std::string_view level_name(LogLevel level);

struct LogLine {
  LogLevel level = LogLevel::Info;
  std::string text;
};

// The newest `capacity` log lines, oldest first. Script console.* calls and
// script errors land here so the dev tools can show them.
class LogRing {
 public:
  explicit LogRing(std::size_t capacity);
  void push(LogLevel level, std::string text);
  void clear();
  std::size_t size() const { return lines_.size(); }
  const LogLine& at(std::size_t i) const { return lines_.at(i); }
  // Lines ever pushed, including dropped ones.
  std::size_t total() const { return total_; }

 private:
  std::size_t capacity_;
  std::deque<LogLine> lines_;
  std::size_t total_ = 0;
};

}  // namespace alpha::core
