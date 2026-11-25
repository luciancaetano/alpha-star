#include <doctest/doctest.h>

#include "core/log_ring.hpp"

using alpha::core::LogLevel;
using alpha::core::LogRing;

TEST_CASE("log ring keeps the newest lines in order and drops the oldest") {
  LogRing log(3);
  log.push(LogLevel::Info, "a");
  log.push(LogLevel::Warn, "b");
  CHECK(log.size() == 2);
  log.push(LogLevel::Error, "c");
  log.push(LogLevel::Info, "d");
  REQUIRE(log.size() == 3);
  CHECK(log.at(0).text == "b");
  CHECK(log.at(0).level == LogLevel::Warn);
  CHECK(log.at(2).text == "d");
  CHECK(log.total() == 4);
}

TEST_CASE("log ring clear empties it; level names are readable") {
  LogRing log(2);
  log.push(LogLevel::Error, "x");
  log.clear();
  CHECK(log.size() == 0);
  CHECK(alpha::core::level_name(LogLevel::Info) == "log");
  CHECK(alpha::core::level_name(LogLevel::Warn) == "warn");
  CHECK(alpha::core::level_name(LogLevel::Error) == "error");
}

TEST_CASE("a log line reads as it does on stderr and in the shipping log file") {
  CHECK(alpha::core::format_line({LogLevel::Warn, "low fuel"}) == "[warn] low fuel\n");
  CHECK(alpha::core::format_line({LogLevel::Info, ""}) == "[log] \n");
}
