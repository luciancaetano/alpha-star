#include <doctest/doctest.h>

#include <cmath>
#include <functional>

#include "devtools/shake_toggle.hpp"

using alpha::devtools::ShakeToggle;

namespace {

constexpr float kG = 9.81f;   // phone held upright: gravity on +y
constexpr float kPi = 3.14159265f;
constexpr std::uint64_t kStep = 10;  // 100 Hz sensor

struct Accel {
  float x, y, z;
};

// Feeds `motion(t)` (t in ms since `from`) for `ms`; returns how many times it toggled.
int feed(ShakeToggle& s, std::uint64_t from, std::uint64_t ms, const std::function<Accel(std::uint64_t)>& motion) {
  int toggles = 0;
  for (std::uint64_t t = 0; t < ms; t += kStep) {
    const Accel a = motion(t);
    toggles += s.sample(a.x, a.y, a.z, from + t) ? 1 : 0;
  }
  return toggles;
}

Accel rest(std::uint64_t) { return {0, kG, 0}; }

// A deliberate shake: 4 Hz side to side, 3 g peaks.
Accel shaking(std::uint64_t t) { return {30.0f * std::sin(2 * kPi * 4 * static_cast<float>(t) / 1000), kG, 0}; }

}  // namespace

TEST_CASE("shake toggle: a deliberate shake toggles exactly once") {
  ShakeToggle s;
  CHECK(feed(s, 0, 1000, rest) == 0);
  CHECK(feed(s, 1000, 1200, shaking) == 1);  // still shaking inside the cooldown: no second toggle
}

TEST_CASE("shake toggle: after the cooldown a new shake toggles again") {
  ShakeToggle s;
  feed(s, 0, 1000, rest);
  CHECK(feed(s, 1000, 600, shaking) == 1);
  CHECK(feed(s, 1600, ShakeToggle::kCooldownMs, rest) == 0);
  CHECK(feed(s, 1600 + ShakeToggle::kCooldownMs, 600, shaking) == 1);
}

TEST_CASE("shake toggle: holding still or a short shake never toggles") {
  ShakeToggle s;
  CHECK(feed(s, 0, 3000, rest) == 0);
  CHECK(feed(s, 3000, 200, shaking) == 0);  // two jolts: one reversal
  CHECK(feed(s, 3200, 2000, rest) == 0);
}

TEST_CASE("shake toggle: taps on the screen never toggle") {
  ShakeToggle s;
  feed(s, 0, 1000, rest);
  const auto taps = [](std::uint64_t t) -> Accel {
    if (t % 150 == 0) return {0, kG, 40};   // the tap
    if (t % 150 == 10) return {0, kG, -10};  // a small rebound
    return {0, kG, 0};
  };
  CHECK(feed(s, 1000, 5000, taps) == 0);
}

TEST_CASE("shake toggle: tilting the phone back and forth never toggles") {
  ShakeToggle s;
  const auto tilt = [](std::uint64_t t) -> Accel {
    const float angle = kPi / 2 * std::sin(2 * kPi * 2.5f * static_cast<float>(t) / 1000);
    return {kG * std::sin(angle), kG * std::cos(angle), 0};
  };
  CHECK(feed(s, 0, 5000, tilt) == 0);
}

TEST_CASE("shake toggle: walking with hard footsteps never toggles") {
  ShakeToggle s;
  const auto walk = [](std::uint64_t t) -> Accel {
    const float bob = 6.0f * std::sin(2 * kPi * 2 * static_cast<float>(t) / 1000);
    const float step = t % 500 == 0 ? 18.0f : 0.0f;
    return {1.5f * std::sin(2 * kPi * static_cast<float>(t) / 1000), kG + bob + step, 0};
  };
  CHECK(feed(s, 0, 10000, walk) == 0);
}

TEST_CASE("shake toggle: jolts right after a resume do not count") {
  ShakeToggle s;
  feed(s, 0, 1000, rest);
  CHECK(feed(s, 3000, ShakeToggle::kSettleMs - 20, shaking) == 0);  // sensor back after a pause
  CHECK(feed(s, 3000 + ShakeToggle::kSettleMs, 1000, shaking) == 1);
}
