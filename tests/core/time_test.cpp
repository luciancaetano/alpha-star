#include <doctest/doctest.h>

#include <vector>

#include "core/error.hpp"
#include "core/fixed_step.hpp"
#include "core/timers.hpp"

using alpha::core::Error;
using alpha::core::FixedStep;
using alpha::core::TimerId;
using alpha::core::Timers;

TEST_CASE("FixedStep turns real time into whole steps and keeps the remainder") {
  FixedStep fs(0.25, 8);
  CHECK(fs.advance(0.1) == 0);
  CHECK(fs.advance(0.2) == 1);  // 0.3 total -> 1 step, 0.05 left
  CHECK(fs.advance(0.45) == 2);
  CHECK(fs.dt() == 0.25);
}

TEST_CASE("FixedStep fraction is the fraction of the next step already elapsed") {
  FixedStep fs(0.25, 8);
  CHECK(fs.fraction() == 0.0);
  fs.advance(0.3);  // 1 step, 0.05 left
  CHECK(fs.fraction() == doctest::Approx(0.2));
  fs.advance(0.15);  // 0.2 left, no step
  CHECK(fs.fraction() == doctest::Approx(0.8));
  fs.advance(10.0);  // stall: backlog dropped
  CHECK(fs.fraction() == 0.0);
}

TEST_CASE("FixedStep caps steps after a stall and drops the backlog") {
  FixedStep fs(0.25, 3);
  CHECK(fs.advance(10.0) == 3);
  CHECK(fs.advance(0.0) == 0);
}

TEST_CASE("FixedStep reset drops the accumulated time: resuming never bursts") {
  FixedStep fs(0.25, 8);
  fs.advance(0.2);
  fs.reset();
  CHECK(fs.fraction() == 0.0);
  CHECK(fs.advance(0.1) == 0);
}

TEST_CASE("Timers: after fires once, every repeats") {
  Timers t;
  const TimerId once = t.after(1.0);
  const TimerId rep = t.every(0.5);
  CHECK(t.advance(0.5) == std::vector<TimerId>{rep});
  CHECK(t.advance(0.5) == std::vector<TimerId>{once, rep});
  CHECK(t.advance(1.0) == std::vector<TimerId>{rep, rep});
}

TEST_CASE("Timers: cancel and clear stop firing") {
  Timers t;
  const TimerId a = t.every(1.0);
  const TimerId b = t.every(1.0);
  t.cancel(a);
  CHECK(t.advance(1.0) == std::vector<TimerId>{b});
  t.clear();
  CHECK(t.advance(5.0).empty());
}

TEST_CASE("Timers reject non-positive periods") {
  Timers t;
  CHECK_THROWS_AS(t.every(0.0), Error);
  CHECK_THROWS_AS(t.after(-1.0), Error);
  // Fuzz finding: every(5e-324) made one advance collect ~1e322 firings (bad_alloc).
  CHECK_THROWS_WITH(t.every(5e-324), doctest::Contains(">= 0.001"));
  CHECK_THROWS_AS(t.after(std::nan("")), Error);
  CHECK_NOTHROW(t.every(0.001));
}
