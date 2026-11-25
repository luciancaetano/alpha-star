#include <doctest/doctest.h>

#include "devtools/touch_toggle.hpp"

using alpha::devtools::TouchToggle;

namespace {

TouchToggle three_down(std::uint64_t ms) {
  TouchToggle t;
  t.down(1, 0.2f, 0.5f, ms);
  t.down(2, 0.5f, 0.5f, ms);
  t.down(3, 0.8f, 0.5f, ms);
  return t;
}

}  // namespace

TEST_CASE("touch toggle: three fingers held still for a second toggle once") {
  TouchToggle t = three_down(100);
  CHECK_FALSE(t.poll(1099));
  CHECK(t.poll(1100));
  CHECK_FALSE(t.poll(5000));  // once per gesture, however long it is held
}

TEST_CASE("touch toggle: lifting all fingers arms it again") {
  TouchToggle t = three_down(0);
  CHECK(t.poll(1000));
  t.up(1);
  t.up(2);
  t.up(3);
  t.down(4, 0.1f, 0.1f, 2000);
  t.down(5, 0.2f, 0.1f, 2000);
  t.down(6, 0.3f, 0.1f, 2000);
  CHECK(t.poll(3000));
}

TEST_CASE("touch toggle: two fingers never toggle") {
  TouchToggle t;
  t.down(1, 0.2f, 0.5f, 0);
  t.down(2, 0.5f, 0.5f, 0);
  CHECK_FALSE(t.poll(10000));
}

TEST_CASE("touch toggle: a finger lifted before the second cancels") {
  TouchToggle t = three_down(0);
  t.up(2);
  CHECK_FALSE(t.poll(1000));
}

TEST_CASE("touch toggle: moving fingers (a game swipe) cancel it") {
  TouchToggle t = three_down(0);
  t.move(1, 0.2f, 0.6f);
  CHECK_FALSE(t.poll(1000));
}

TEST_CASE("touch toggle: a small wobble still counts as still") {
  TouchToggle t = three_down(0);
  t.move(1, 0.21f, 0.51f);
  CHECK(t.poll(1000));
}
