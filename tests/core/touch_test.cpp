#include <doctest/doctest.h>

#include "core/touch.hpp"

using alpha::core::parse_input;
using alpha::core::Touches;

namespace {

bool holds(const Touches& t, const char* name) { return t.held().test(*parse_input(name)); }

}  // namespace

TEST_CASE("touch inputs: Touch while any finger is down, TouchLeft/TouchRight by screen half") {
  Touches t;
  CHECK(t.held().none());
  t.down(1, 0.2f);
  CHECK(holds(t, "Touch"));
  CHECK(holds(t, "TouchLeft"));
  CHECK_FALSE(holds(t, "TouchRight"));
  t.down(2, 0.9f);
  CHECK(holds(t, "TouchRight"));
  t.up(1);
  CHECK_FALSE(holds(t, "TouchLeft"));
  CHECK(holds(t, "Touch"));
  t.move(2, 0.1f);  // dragged across the middle
  CHECK(holds(t, "TouchLeft"));
  CHECK_FALSE(holds(t, "TouchRight"));
  t.down(2, 0.7f);  // the same finger again: still one finger
  t.up(2);
  CHECK(t.held().none());
  t.up(9);  // unknown finger: nothing happens
  CHECK(t.held().none());
}

TEST_CASE("touch and back are bindable input names") {
  for (const char* name : {"Touch", "TouchLeft", "TouchRight", "Back"}) CHECK(parse_input(name).has_value());
}
