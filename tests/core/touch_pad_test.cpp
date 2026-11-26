#include <doctest/doctest.h>

#include <cmath>
#include <string>
#include <vector>

#include "core/controls.hpp"
#include "core/error.hpp"
#include "core/suggest.hpp"
#include "core/touch_pad.hpp"

using namespace alpha::core;

namespace {

// A 640x360 screen with no cutouts.
const TouchScreen kScreen{{{0, 0}, {640, 360}}, {640, 360}};

StickConfig stick(const char* name) {
  StickConfig s;
  s.name = name;
  return s;
}

ButtonConfig button(const char* name, Vec2 pos) {
  ButtonConfig b;
  b.name = name;
  b.pos = pos;
  return b;
}

Finger at(std::uint64_t id, Vec2 pos, bool began = false, bool ended = false) { return {id, pos, began, ended}; }

bool holds(const TouchPad& pad, const TouchFrame& f, const char* name, std::size_t player = 0) {
  return f.held[player].test(*pad.input(name));
}

// Controls with a "move" stick on the left and a "jump" button, bound like a game would.
Controls game(bool mobile = true) {
  Controls c;
  c.touch.configure({stick("move")}, {button("jump", {0.9f, 0.8f})}, mobile);
  c.touch.set_screen(kScreen);
  const std::vector<std::string> x{"A/D", "TouchMoveX"};
  c.axes.bind("moveX", x, &c.touch);
  const std::vector<std::string> jump{"Space", "TouchJump"};
  c.actions.bind("jump", jump, &c.touch);
  return c;
}

InputFrame fingers(std::vector<Finger> f) {
  InputFrame frame;
  frame.fingers = std::move(f);
  return frame;
}

}  // namespace

TEST_CASE("stick value: offset over radius, capped at 1, radial deadzone remapped") {
  StickConfig s = stick("move");
  CHECK(stick_value({0, 0}, s) == Vec2{0, 0});
  CHECK(stick_value({6, 0}, s) == Vec2{0, 0});  // 6/64 < 0.15
  CHECK(stick_value({64, 0}, s).x == doctest::Approx(1.0));
  CHECK(stick_value({200, 0}, s).x == doctest::Approx(1.0));
  const float half = (0.5f - 0.15f) / (1.0f - 0.15f);
  CHECK(stick_value({0, -32}, s).y == doctest::Approx(-half));
}

TEST_CASE("stick value: axes keep one axis, snap quantizes the direction") {
  StickConfig s = stick("move");
  s.deadzone = 0.0f;
  s.axes = StickAxes::X;
  CHECK(stick_value({32, 32}, s).y == 0.0f);
  s.axes = StickAxes::Both;
  s.snap = 4;
  const Vec2 v = stick_value({60, 20}, s);  // mostly right
  CHECK(v.y == 0.0f);
  CHECK(v.x > 0.9f);
  s.snap = 8;
  const Vec2 d = stick_value({40, 40}, s);
  CHECK(d.x == doctest::Approx(d.y));
}

TEST_CASE("a finger in the zone drives the stick: axis, direction inputs, Active; lifting zeroes it the same step") {
  Controls c = game();
  c.update(fingers({at(1, {100, 200}, true)}));
  CHECK(c.touch.stick_states()[0].finger == 1u);
  CHECK(c.actions.find("jump"));
  c.update(fingers({at(1, {164, 200})}));  // floating: base where it landed, now a full radius right
  const auto id = *c.axes.find("moveX");
  CHECK(c.axes.value(id) == doctest::Approx(1.0));
  const TouchFrame held = c.touch.update({at(1, {164, 200})});
  CHECK(holds(c.touch, held, "TouchMoveX+"));
  CHECK(holds(c.touch, held, "TouchMoveActive"));
  CHECK_FALSE(holds(c.touch, held, "Touch"));  // a captured finger is not a free touch
  c.update(fingers({at(1, {164, 200}, false, true)}));
  CHECK(c.axes.value(id) == 0.0f);
  CHECK_FALSE(c.touch.stick_states()[0].finger);
}

TEST_CASE("buttons: down until their finger lifts (dragging off keeps it), a short tap still presses") {
  Controls c = game();
  const auto jump = *c.actions.find("jump");
  c.update(fingers({at(2, {576, 288}, true)}));
  CHECK(c.actions.pressed(jump));
  c.update(fingers({at(2, {10, 10})}));
  CHECK(c.actions.down(jump));
  c.update(fingers({at(2, {10, 10}, false, true)}));
  CHECK(c.actions.released(jump));
  c.update(fingers({at(3, {576, 288}, true, true)}));
  CHECK(c.actions.pressed(jump));
}

TEST_CASE("capture: a button inside a stick zone wins; one finger per control; extras are free touches") {
  TouchPad pad;
  pad.configure({stick("move")}, {button("fire", {0.2f, 0.5f})}, true);
  pad.set_screen(kScreen);
  TouchFrame f = pad.update({at(1, {128, 180}, true), at(2, {300, 100}, true), at(3, {500, 100}, true)});
  CHECK(holds(pad, f, "TouchFire"));
  CHECK(pad.stick_states()[0].finger == 2u);
  REQUIRE(f.free.size() == 1);
  CHECK(f.free[0].id == 3);
  CHECK(holds(pad, f, "Touch"));
  CHECK(holds(pad, f, "TouchRight"));
  CHECK(holds(pad, f, "MouseLeft"));
}

TEST_CASE("desktop: the pad is inert, every finger is a free touch") {
  Controls c = game(false);
  c.update(fingers({at(1, {100, 200}, true)}));
  CHECK_FALSE(c.touch.stick_states()[0].finger);
  REQUIRE(c.fingers.size() == 1);
  CHECK(c.mouse == Vec2{100, 200});
}

TEST_CASE("release_all (pause): controls let go and their fingers drive nothing until lifted") {
  Controls c = game();
  c.update(fingers({at(1, {100, 200}, true)}));
  c.touch.release_all();
  c.update(fingers({at(1, {164, 200})}));
  CHECK(c.axes.value(*c.axes.find("moveX")) == 0.0f);
  CHECK(c.fingers.empty());
  c.update(fingers({at(1, {164, 200}, false, true)}));
  c.update(fingers({at(4, {100, 200}, true)}));
  CHECK(c.touch.stick_states()[0].finger == 4u);
}

TEST_CASE("scenes: a control exists only in the scenes it lists; a new scene releases everything") {
  StickConfig s = stick("move");
  s.scenes = {"play"};
  TouchPad pad;
  pad.configure({s}, {}, true);
  pad.set_screen(kScreen);
  pad.enter_scene("title");
  CHECK_FALSE(pad.here(s.scenes));
  TouchFrame f = pad.update({at(1, {100, 200}, true)});
  CHECK_FALSE(pad.stick_states()[0].finger);
  CHECK(f.free.size() == 1);
  pad.enter_scene("play");
  CHECK(pad.here(s.scenes));
  pad.update({at(2, {100, 200}, true)});
  CHECK(pad.stick_states()[0].finger == 2u);
  pad.enter_scene("over");
  CHECK_FALSE(pad.stick_states()[0].finger);
}

TEST_CASE("a finger that vanishes without lifting lets go") {
  Controls c = game();
  c.update(fingers({at(1, {100, 200}, true)}));
  c.update(fingers({}));
  CHECK_FALSE(c.touch.stick_states()[0].finger);
}

TEST_CASE("fixed sticks sit at their anchor; floating ones fit inside the safe area") {
  StickConfig s = stick("aim");
  s.zone = {{0.5f, 0.0f}, {1.0f, 1.0f}};
  CHECK(stick_center(s, kScreen, {630, 350}) == Vec2{576, 296});
  s.mode = StickMode::Fixed;
  CHECK(stick_center(s, kScreen, {630, 350}) == Vec2{480, 180});
  s.anchor = Vec2{0.75f, 0.5f};
  CHECK(stick_anchor(s, kScreen) == Vec2{480, 180});
}

TEST_CASE("names: sources from PascalCase names; errors for bad, taken and too many") {
  TouchPad pad;
  pad.configure({stick("move")}, {button("jump", {0.5f, 0.5f})}, true);
  CHECK(pad.axis("TouchMoveX") == 0u);
  CHECK(pad.axis("TouchMoveY") == 1u);
  CHECK(pad.input("TouchJump").has_value());
  CHECK(pad.input("Space") == parse_input("Space"));
  CHECK_FALSE(pad.input("TouchMovX"));
  CHECK(pad.axis_names() == "TouchMoveX, TouchMoveY");
  CHECK_THROWS_AS(pad.configure({stick("move")}, {button("move", {0, 0})}, true), Error);
  CHECK_THROWS_AS(pad.configure({}, {button("left", {0, 0})}, true), Error);  // TouchLeft is a fixed input
  CHECK_THROWS_AS(pad.configure({stick("a b")}, {}, true), Error);
  CHECK_THROWS_AS(pad.configure({stick("a"), stick("b"), stick("c"), stick("d"), stick("e")}, {}, true), Error);
}

TEST_CASE("axis and action errors list the touch sources") {
  Controls c = game();
  const std::vector<std::string> bad{"TouchMovX"};
  try {
    c.axes.bind("moveY", bad, &c.touch);
    FAIL("expected an error");
  } catch (const Error& e) {
    CHECK(std::string(e.what()).find("TouchMoveX, TouchMoveY") != std::string::npos);
  }
  CHECK_THROWS_AS(c.actions.bind("fire", bad, &c.touch), Error);
}

TEST_CASE("buttons overlapping by radius + hitSlop are detected") {
  CHECK(buttons_overlap(button("a", {0.5f, 0.5f}), button("b", {0.55f, 0.5f}), kScreen));
  CHECK_FALSE(buttons_overlap(button("a", {0.1f, 0.5f}), button("b", {0.9f, 0.5f}), kScreen));
}

TEST_CASE("closest: a near name for did-you-mean hints, none when far") {
  const std::vector<std::string> names{"ui/stick_knob", "ui/stick_base"};
  CHECK(closest("ui/stick_knb", names) == "ui/stick_knob");
  CHECK_FALSE(closest("player", names));
}
