#include <doctest/doctest.h>

#include <thread>

#include "core/perf.hpp"

using alpha::core::FrameTimes;
using alpha::core::PerfHistory;
using alpha::core::Section;
using alpha::core::spread_of;

TEST_CASE("spread is nearest-rank p50, p95 and max") {
  std::deque<double> values;
  for (int i = 100; i >= 1; --i) values.push_back(i);
  const auto s = spread_of(values);
  CHECK(s.p50 == 50.0);
  CHECK(s.p95 == 95.0);
  CHECK(s.max == 100.0);
  CHECK(spread_of({}).max == 0.0);
  CHECK(spread_of({7.0}).p50 == 7.0);
}

TEST_CASE("history keeps the newest frames and spreads one section") {
  PerfHistory history(3);
  for (int i = 1; i <= 5; ++i) {
    FrameTimes f;
    f[Section::physics] = i;
    f[Section::render] = 1.0;
    history.push(f);
  }
  REQUIRE(history.size() == 3);
  CHECK(history.at(0)[Section::physics] == 3.0);
  CHECK(history.spread(Section::physics).max == 5.0);
  CHECK(history.spread(Section::physics).p50 == 4.0);
  CHECK(history.total().max == 6.0);
  const std::string json = alpha::core::to_json(history);
  CHECK(json.starts_with("{\"frames\":3,\"total\":{\"p50\":5.0000,"));
  CHECK(json.find("\"physics\":{\"p50\":4.0000,\"p95\":5.0000,\"max\":5.0000}") != std::string::npos);
  CHECK(json.ends_with("\"audio\":{\"p50\":0.0000,\"p95\":0.0000,\"max\":0.0000}}"));
}

TEST_CASE("a scoped time adds the scope's milliseconds") {
  double ms = 1.0;
  {
    alpha::core::ScopedTime t(ms);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  if constexpr (alpha::core::kProfile) CHECK(ms >= 2.5);
  else CHECK(ms == 1.0);
}
