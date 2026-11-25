#include <doctest/doctest.h>

#include <cmath>
#include <optional>
#include <string>
#include <vector>

#include "core/axes.hpp"
#include "core/controls.hpp"
#include "core/error.hpp"
#include "core/pad_slots.hpp"

using namespace alpha::core;

namespace {

InputFrame stick(std::size_t player, float x, float y) {
  InputFrame f;
  f.players[player].analog[0] = x;
  f.players[player].analog[1] = y;
  return f;
}

std::string error_of(Axes& axes, const std::vector<std::string>& sources) {
  try {
    axes.bind("moveX", sources);
  } catch (const Error& e) {
    return e.what();
  }
  return "";
}

}  // namespace

TEST_CASE("axial deadzone zeroes small values and rescales the rest to reach 1") {
  CHECK(axial_deadzone(0.1f, 0.2f) == 0.0f);
  CHECK(axial_deadzone(-0.2f, 0.2f) == 0.0f);
  CHECK(axial_deadzone(0.6f, 0.2f) == doctest::Approx(0.5f));
  CHECK(axial_deadzone(-1.0f, 0.2f) == doctest::Approx(-1.0f));
  CHECK(axial_deadzone(1.5f, 0.2f) == doctest::Approx(1.0f));
}

TEST_CASE("radial deadzone keeps the direction and judges the length") {
  const auto small = radial_deadzone(0.15f, 0.15f, 0.25f);  // length 0.21
  CHECK(small[0] == 0.0f);
  CHECK(small[1] == 0.0f);
  const auto diag = radial_deadzone(0.3f, 0.3f, 0.25f);  // length 0.42
  CHECK(diag[0] > 0.0f);
  CHECK(diag[0] == doctest::Approx(diag[1]));
  const auto full = radial_deadzone(1.0f, 1.0f, 0.2f);
  CHECK(std::hypot(full[0], full[1]) == doctest::Approx(1.0f));
}

TEST_CASE("with_deadzone applies radial to sticks and axial to triggers") {
  AnalogValues raw{0.1f, 0.1f, 0.0f, 1.0f, 0.1f, 0.6f};
  const AnalogValues out = with_deadzone(raw, 0.2f);
  CHECK(out[0] == 0.0f);
  CHECK(out[3] == doctest::Approx(1.0f));
  CHECK(out[4] == 0.0f);
  CHECK(out[5] == doctest::Approx(0.5f));
}

TEST_CASE("analog axes also read as digital inputs past the threshold") {
  AnalogValues raw{-0.7f, 0.2f, 0.0f, 0.9f, 0.0f, 0.8f};
  const InputSet held = analog_buttons(raw);
  CHECK(held.test(*parse_input("GamepadLeftX-")));
  CHECK_FALSE(held.test(*parse_input("GamepadLeftX+")));
  CHECK_FALSE(held.test(*parse_input("GamepadLeftY+")));
  CHECK(held.test(*parse_input("GamepadRightY+")));
  CHECK(held.test(*parse_input("GamepadRT")));
  CHECK_FALSE(held.test(*parse_input("GamepadLT")));
}

TEST_CASE("an axis reads a stick after the deadzone, or a key pair as a fallback") {
  Axes axes;
  const std::vector<std::string> sources{"GamepadLeftX", "A/D", "Left/Right"};
  const AxisId x = axes.bind("moveX", sources);
  CHECK(axes.find("moveX") == x);

  axes.update(stick(0, 0.6f, 0.0f));
  CHECK(axes.value(x) == doctest::Approx(0.5f));

  InputFrame keys;
  keys.players[0].held.set(*parse_input("A"));
  axes.update(keys);
  CHECK(axes.value(x) == -1.0f);

  keys.players[0].held.set(*parse_input("D"));  // both: cancel out
  axes.update(keys);
  CHECK(axes.value(x) == 0.0f);
}

TEST_CASE("an axis reads per player; any player is the largest magnitude") {
  Axes axes;
  const std::vector<std::string> sources{"GamepadLT"};
  const AxisId t = axes.bind("throttle", sources);
  InputFrame f;
  f.players[1].analog[4] = 0.6f;
  f.players[2].analog[4] = 1.0f;
  axes.update(f);
  CHECK(axes.value(t, 0) == 0.0f);
  CHECK(axes.value(t, 1) == doctest::Approx(0.5f));
  CHECK(axes.value(t) == doctest::Approx(1.0f));
  axes.set(t, 3, -2.0f);
  CHECK(axes.value(t, 3) == -1.0f);
}

TEST_CASE("a configured deadzone applies and bad ones are rejected") {
  Axes axes;
  const std::vector<std::string> sources{"GamepadLeftY"};
  const AxisId y = axes.bind("moveY", sources);
  axes.set_deadzone(0.0f);
  axes.update(stick(0, 0.0f, 0.1f));
  CHECK(axes.value(y) == doctest::Approx(0.1f));
  CHECK_THROWS_AS(axes.set_deadzone(1.0f), Error);
  CHECK_THROWS_AS(axes.set_deadzone(-0.1f), Error);
}

TEST_CASE("bad axis sources name the source and the accepted forms") {
  Axes axes;
  const std::string unknown = error_of(axes, {"Stick"});
  CHECK(unknown.find("\"Stick\"") != std::string::npos);
  CHECK(unknown.find("GamepadLeftX") != std::string::npos);
  CHECK(unknown.find("A/D") != std::string::npos);
  CHECK(error_of(axes, {"A/Nope"}).find("\"Nope\"") != std::string::npos);
  CHECK(error_of(axes, {"A/D/W"}) != "");
  CHECK_FALSE(axes.find("moveX"));
  const std::vector<std::string> ok{"A/D"};
  axes.bind("moveX", ok);
  axes.bind("moveY", ok);
  CHECK(axes.names() == "moveX moveY");
}

TEST_CASE("Controls update actions, axes and the player count together") {
  Controls c;
  const std::vector<std::string> up{"GamepadLeftY-"};
  const ActionId jump = c.actions.bind("jump", up);
  InputFrame f = stick(1, 0.0f, -0.9f);
  f.connected = 2;
  c.update(f);
  CHECK(c.actions.pressed(jump));
  CHECK(c.actions.pressed(jump, 1));
  CHECK_FALSE(c.actions.pressed(jump, 0));
  CHECK(c.players == 2);
}

TEST_CASE("rumble requests are validated and kept per player until taken") {
  Controls c;
  c.request_rumble(0.5f, 0.2f, 1);
  c.request_rumble(1.0f, 1.0f, kAnyPlayer);  // every player; the latest request wins
  c.request_rumble(0.25f, 0.1f, 2);
  const auto taken = c.take_rumble();
  REQUIRE(taken[0]);
  CHECK(taken[0]->strength == 1.0f);
  CHECK(taken[2]->strength == 0.25f);
  CHECK(taken[2]->seconds == doctest::Approx(0.1f));
  CHECK_FALSE(c.take_rumble()[0]);
  CHECK_THROWS_AS(c.request_rumble(1.5f, 0.1f, 0), Error);
  CHECK_THROWS_AS(c.request_rumble(0.5f, -1.0f, 0), Error);
  CHECK_THROWS_AS(c.request_rumble(0.5f, 11.0f, 0), Error);
}

TEST_CASE("gamepads get stable slots in connection order and reclaim them on reconnect") {
  PadSlots slots;
  CHECK(slots.players() == 1);
  CHECK(slots.connect(10, "xbox") == 0u);
  CHECK(slots.connect(11, "ps") == 1u);
  CHECK(slots.connect(10, "xbox") == 0u);  // idempotent
  CHECK(slots.players() == 2);
  CHECK(slots.disconnect(10) == 0u);
  CHECK_FALSE(slots.slot_of(10));
  CHECK(slots.players() == 2);
  CHECK(slots.connect(12, "switch") == 2u);  // never-used slot first: 0 stays reserved
  CHECK(slots.connect(13, "xbox") == 0u);    // same kind reconnects: slot 0 again
  CHECK(slots.slot_of(11) == 1u);
  CHECK(slots.connect(14, "other") == 3u);
  CHECK_FALSE(slots.connect(15, "fifth"));
  CHECK_FALSE(slots.disconnect(99));
  CHECK(slots.disconnect(14) == 3u);
  CHECK(slots.players() == 3);
}

TEST_CASE("a new device takes a free reserved slot when no slot is unused") {
  PadSlots slots;
  for (std::uint32_t i = 0; i < kMaxPlayers; ++i) slots.connect(i + 1, "k" + std::to_string(i));
  slots.disconnect(2);
  CHECK(slots.connect(50, "new") == 1u);
}

TEST_CASE("a held axis value survives updates until released (agent input)") {
  Axes axes;
  const std::vector<std::string> sources{"A/D"};
  const AxisId x = axes.bind("moveX", sources);
  axes.hold(x, 1, 0.5f);
  axes.update(InputFrame{});
  CHECK(axes.value(x, 1) == doctest::Approx(0.5f));
  CHECK(axes.value(x, 0) == 0.0f);
  axes.update(InputFrame{});
  CHECK(axes.value(x, 1) == doctest::Approx(0.5f));
  axes.hold(x, 1, 3.0f);
  axes.update(InputFrame{});
  CHECK(axes.value(x, 1) == 1.0f);
  axes.hold(x, 1, std::nullopt);
  axes.update(InputFrame{});
  CHECK(axes.value(x, 1) == 0.0f);
}

TEST_CASE("actions list their names for error messages") {
  Actions actions;
  CHECK(actions.names().empty());
  const std::vector<std::string> keys{"Space"};
  actions.bind("flap", keys);
  actions.bind("pause", keys);
  CHECK(actions.names() == "flap pause");
}
