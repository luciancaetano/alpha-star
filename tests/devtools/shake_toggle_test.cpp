#include <doctest/doctest.h>

#include "devtools/shake_toggle.hpp"

using alpha::devtools::ShakeToggle;

namespace {

constexpr float kRest = 9.81f;   // phone lying still: gravity only
constexpr float kJolt = 20.0f;   // a hard shake

// One shake = a jolt sample then a rest sample, `gap` ms apart; returns whether any toggled.
bool shake(ShakeToggle& s, int times, std::uint64_t start, std::uint64_t gap) {
  bool fired = false;
  for (int i = 0; i < times; ++i) {
    fired |= s.sample(0, kJolt, 0, start + 2 * i * gap);
    fired |= s.sample(0, kRest, 0, start + (2 * i + 1) * gap);
  }
  return fired;
}

}  // namespace

TEST_CASE("shake toggle: a few hard shakes within a second toggle once") {
  ShakeToggle s;
  CHECK(shake(s, ShakeToggle::kShakes, 0, 50));
}

TEST_CASE("shake toggle: fewer shakes, or holding still, never toggle") {
  ShakeToggle s;
  CHECK_FALSE(shake(s, ShakeToggle::kShakes - 1, 0, 50));
  CHECK_FALSE(s.sample(0, kRest, 0, 5000));
}

TEST_CASE("shake toggle: shakes spread over more than the window do not add up") {
  ShakeToggle s;
  CHECK_FALSE(shake(s, ShakeToggle::kShakes, 0, 400));
}

TEST_CASE("shake toggle: a jolt held high counts once, not every sample") {
  ShakeToggle s;
  bool fired = false;
  for (int i = 0; i < 20; ++i) fired |= s.sample(kJolt, 0, 0, static_cast<std::uint64_t>(i) * 10);
  CHECK_FALSE(fired);
}

TEST_CASE("shake toggle: after a toggle the shaking must start over") {
  ShakeToggle s;
  CHECK(shake(s, ShakeToggle::kShakes, 0, 50));
  CHECK_FALSE(shake(s, 1, 500, 50));
  CHECK(shake(s, ShakeToggle::kShakes, 2000, 50));
}
