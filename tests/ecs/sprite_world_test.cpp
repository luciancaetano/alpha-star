#include <doctest/doctest.h>

#include <memory>

#include "core/error.hpp"
#include "ecs/world.hpp"

using namespace alpha::core;
using alpha::ecs::EntityId;
using alpha::ecs::World;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// 4 frames of 8x8 in a 32x8 image: run = 0,1,2 at 10 fps (looping), jump = 3 once.
SheetPtr hero_sheet() {
  auto s = std::make_shared<Sheet>();
  s->picture = {std::make_shared<Image>(Image{32, 8, std::vector<std::uint8_t>(32 * 8 * 4)}), {0, 0, 32, 8}};
  s->frame_w = 8;
  s->frame_h = 8;
  s->animations["run"] = {{0, 1, 2}, 10.0f, true};
  s->animations["jump"] = {{3}, 10.0f, false};
  return s;
}

EntitySpec animated(const SheetPtr& sheet, const std::string& anim) {
  EntitySpec spec{{}, {0, 0}, 0.0f, 0, ShapeSpec{Rect{{8, 8}}, {}, {}, sheet->picture.image}, {}};
  spec.shape->animation = AnimationSpec{sheet, anim};
  return spec;
}

Region src_of(const World& w) { return w.draw_items().at(0).shape.src; }

void steps(World& w, int n) {
  for (int i = 0; i < n; ++i) {
    w.snapshot();
    w.step(kDt);
  }
}

}  // namespace

TEST_CASE("an animated entity shows its frames as fixed steps pass") {
  World w({0, 0});
  const EntityId e = w.spawn(animated(hero_sheet(), "run"));
  CHECK(w.animation(e) == "run");
  CHECK(src_of(w) == Region{0, 0, 8, 8});
  steps(w, 6);  // 0.1 s at 10 fps
  CHECK(src_of(w) == Region{8, 0, 8, 8});
  steps(w, 12);  // 0.3 s: wrapped to frame 0
  CHECK(src_of(w) == Region{0, 0, 8, 8});
}

TEST_CASE("play switches animation; same name keeps going unless restarted") {
  World w({0, 0});
  const EntityId e = w.spawn(animated(hero_sheet(), "run"));
  steps(w, 6);
  w.play(e, "run", false);
  CHECK(src_of(w) == Region{8, 0, 8, 8});
  w.play(e, "run", true);
  CHECK(src_of(w) == Region{0, 0, 8, 8});
  w.play(e, "jump", false);
  CHECK(w.animation(e) == "jump");
  CHECK(src_of(w) == Region{24, 0, 8, 8});
}

TEST_CASE("unknown animations and entities without a sheet are errors that list what exists") {
  World w({0, 0});
  const EntityId e = w.spawn(animated(hero_sheet(), "run"));
  CHECK_THROWS_WITH_AS(w.play(e, "fly", false), doctest::Contains("jump, run"), Error);
  const EntityId plain = w.spawn({{}, {0, 0}, 0.0f, 0, ShapeSpec{Circle{3}, {}, {}, {}}, {}});
  CHECK(w.animation(plain).empty());
  CHECK_THROWS_WITH_AS(w.play(plain, "run", false), doctest::Contains("sheet"), Error);
}

TEST_CASE("flip mirrors a shape") {
  World w({0, 0});
  const EntityId e = w.spawn(animated(hero_sheet(), "run"));
  w.set_flip(e, true, false);
  CHECK(w.flip(e) == std::pair{true, false});
  CHECK(w.draw_items()[0].shape.flip_x);
  const EntityId shapeless = w.spawn({{}, {0, 0}, 0.0f, 0, {}, {}});
  CHECK_THROWS_AS(w.set_flip(shapeless, true, true), Error);
}

TEST_CASE("the camera follows an entity on the step, and the view blends between steps") {
  World w({0, 0}, {100, 100});
  CHECK(w.view().center == Vec2{50, 50});
  const EntityId e = w.spawn({{}, {300, 50}, 0.0f, 0, {}, {}});
  w.camera().follow = Follow{1.0f, {}};
  w.follow(e);
  w.snapshot();
  w.step(kDt);
  CHECK(w.view().center == Vec2{300, 50});
  CHECK(w.view(0.5f).center == Vec2{175, 50});
  w.destroy(e);
  steps(w, 2);  // a dead target stops the follow; the camera stays
  CHECK(w.view().center == Vec2{300, 50});
  CHECK_FALSE(w.following());
}
