#include <doctest/doctest.h>

#include "devtools/time_control.hpp"

using alpha::devtools::TimeControl;

TEST_CASE("running time control passes the clock's steps through") {
  TimeControl t;
  CHECK_FALSE(t.paused());
  CHECK(t.steps(3) == 3);
}

TEST_CASE("paused time control runs nothing, except one step per request") {
  TimeControl t;
  t.toggle_pause();
  CHECK(t.paused());
  CHECK(t.steps(2) == 0);
  t.request_step();
  t.request_step();  // requests do not pile up: one step per frame
  CHECK(t.steps(0) == 1);
  CHECK(t.steps(4) == 0);
  t.toggle_pause();
  CHECK(t.steps(2) == 2);
}

TEST_CASE("a step request while running is ignored") {
  TimeControl t;
  t.request_step();
  t.toggle_pause();
  CHECK(t.steps(1) == 0);
}
