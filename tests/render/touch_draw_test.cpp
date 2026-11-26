#include <doctest/doctest.h>

#include <memory>
#include <variant>

#include "render/touch_draw.hpp"

using namespace alpha::core;
using alpha::render::touch_overlay;

namespace {

const TouchScreen kScreen{{{0, 0}, {640, 360}}, {640, 360}};

Picture pic(int w) { return {std::make_shared<Image>(Image{w, w, std::vector<std::uint8_t>(static_cast<std::size_t>(w * w * 4)), Filter::nearest}), {0, 0, w, w}}; }

TouchPad pad(StickConfig stick, ButtonConfig button, bool active = true) {
  TouchPad p;
  p.configure({std::move(stick)}, {std::move(button)}, active);
  p.set_screen(kScreen);
  return p;
}

StickConfig stick() {
  StickConfig s;
  s.name = "move";
  return s;
}

ButtonConfig button() {
  ButtonConfig b;
  b.name = "jump";
  b.pos = {0.9f, 0.8f};
  b.label = "A";
  return b;
}

float radius(const DrawItem& item) { return std::get<Circle>(item.shape.geometry).radius; }

}  // namespace

TEST_CASE("default look: a floating stick shows only while touched; buttons show with their label") {
  TouchPad p = pad(stick(), button());
  auto out = touch_overlay(p);
  REQUIRE(out.shapes.size() == 1);  // the button; the floating stick is hidden at rest
  CHECK(out.shapes[0].pos == Vec2{576, 288});
  CHECK(out.shapes[0].shape.alpha == doctest::Approx(0.6));
  REQUIRE(out.labels.size() == 1);
  CHECK(out.labels[0].text == "A");
  p.update({{1, {100, 200}, true, false}});
  p.update({{1, {132, 200}, false, false}});
  out = touch_overlay(p);
  REQUIRE(out.shapes.size() == 3);
  CHECK(out.shapes[0].pos == Vec2{100, 200});  // base where the finger landed
  CHECK(radius(out.shapes[0]) == 64.0f);
  CHECK(out.shapes[1].pos == Vec2{132, 200});  // knob under the finger
  CHECK(radius(out.shapes[1]) == 32.0f);
  CHECK(out.shapes[0].shape.alpha == 1.0f);
}

TEST_CASE("fixed sticks rest at their anchor; desktop draws nothing; hidden buttons draw nothing") {
  StickConfig s = stick();
  s.mode = StickMode::Fixed;
  s.always_visible = true;
  ButtonConfig b = button();
  b.visible = false;
  auto out = touch_overlay(pad(s, b));
  REQUIRE(out.shapes.size() == 2);
  CHECK(out.shapes[0].pos == Vec2{160, 180});
  CHECK(out.labels.empty());
  CHECK(touch_overlay(pad(stick(), button(), false)).shapes.empty());
}

TEST_CASE("skins: images at their sizes, pressed frames while touched, darkened up image without a down one") {
  StickConfig s = stick();
  StickSkin skin;
  skin.base = pic(8);
  skin.knob = pic(4);
  skin.knob_pressed = pic(2);
  skin.base_size = 100.0f;
  s.skin = skin;
  ButtonConfig b = button();
  b.skin = ButtonSkin{};
  b.skin->up = pic(6);
  b.skin->tint = {200, 100, 50, 255};
  TouchPad p = pad(s, b);
  p.update({{1, {100, 200}, true, false}, {2, {576, 288}, true, false}});
  const auto out = touch_overlay(p);
  REQUIRE(out.shapes.size() == 3);
  CHECK(std::get<Rect>(out.shapes[0].shape.geometry).size == Vec2{100, 100});
  CHECK(out.shapes[1].shape.image == skin.knob_pressed->image);
  CHECK(std::get<Rect>(out.shapes[1].shape.geometry).size == Vec2{64, 64});
  CHECK(out.shapes[2].shape.image == b.skin->up.image);
  CHECK(out.shapes[2].shape.tint.r == 120);
}
