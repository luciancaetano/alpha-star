#include <doctest/doctest.h>

#include "core/limits.hpp"
#include "core/touch.hpp"

using alpha::core::Finger;
using alpha::core::parse_input;
using alpha::core::Touches;

TEST_CASE("touches: each step gets its fingers with began / ended, lifted ones then go") {
  Touches t;
  CHECK(t.take().empty());
  t.down(1, {10, 20});
  t.move(1, {15, 20});
  auto step = t.take();
  REQUIRE(step.size() == 1);
  CHECK(step[0].id == 1);
  CHECK(step[0].pos == alpha::core::Vec2{15, 20});
  CHECK(step[0].began);
  CHECK_FALSE(step[0].ended);
  step = t.take();
  REQUIRE(step.size() == 1);
  CHECK_FALSE(step[0].began);  // still down, no longer new
  t.up(1);
  step = t.take();
  REQUIRE(step.size() == 1);
  CHECK(step[0].ended);
  CHECK(t.take().empty());
}

TEST_CASE("touches: a tap shorter than a step arrives with both flags; unknown fingers are ignored") {
  Touches t;
  t.down(3, {1, 2});
  t.up(3);
  t.move(9, {0, 0});
  t.up(9);
  const auto step = t.take();
  REQUIRE(step.size() == 1);
  CHECK(step[0].began);
  CHECK(step[0].ended);
}

TEST_CASE("touches: at most kMaxTouches fingers") {
  Touches t;
  for (std::uint64_t id = 0; id < alpha::core::kMaxTouches + 3; ++id) t.down(id, {0, 0});
  CHECK(t.take().size() == alpha::core::kMaxTouches);
}

TEST_CASE("touch and back are bindable input names") {
  for (const char* name : {"Touch", "TouchLeft", "TouchRight", "Back"}) CHECK(parse_input(name).has_value());
}
