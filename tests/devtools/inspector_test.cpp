#include <doctest/doctest.h>

#include "devtools/inspector.hpp"

using namespace alpha::core;
using alpha::ecs::World;
namespace dev = alpha::devtools;

namespace {

struct Scene {
  TagRegistry tags;
  World world{{0, 0}};
  alpha::ecs::EntityId bird;
  alpha::ecs::EntityId wall;
  Scene() {
    TagSet b;
    b.set(tags.intern("bird"));
    TagSet w;
    w.set(tags.intern("wall"));
    w.set(tags.intern("solid"));
    bird = world.spawn({b, {10, 20}, 0.0f, 2, {}, BodySpec{BodyType::Dynamic, {Circle{5}, false}, {3, 4}}, {}, {}});
    wall = world.spawn({w, {50, 60}, 30.0f, 0, ShapeSpec{Rect{{8, 4}}, Color{}, {}, {}}, {}, {}, {}});
  }
};

}  // namespace

TEST_CASE("inspector lists living entities oldest first with tag names and velocity") {
  Scene s;
  const auto all = dev::list_entities(s.world, s.tags, "");
  REQUIRE(all.size() == 2);
  CHECK(all[0].id == s.bird);
  CHECK(all[0].pos.x == doctest::Approx(10));
  CHECK(all[0].z == 2);
  CHECK(all[0].tags == std::vector<std::string>{"bird"});
  REQUIRE(all[0].velocity);
  CHECK(all[0].velocity->y == doctest::Approx(4));
  CHECK(all[1].rotation == doctest::Approx(30));
  CHECK(all[1].tags == std::vector<std::string>{"wall", "solid"});
  CHECK_FALSE(all[1].velocity);
}

TEST_CASE("inspector filters by tag name substring and skips destroyed entities") {
  Scene s;
  CHECK(dev::list_entities(s.world, s.tags, "sol").size() == 1);
  CHECK(dev::list_entities(s.world, s.tags, "nope").empty());
  s.world.destroy(s.wall);
  CHECK(dev::list_entities(s.world, s.tags, "").size() == 1);
}

TEST_CASE("applying an edited view moves, rotates and pushes the entity") {
  Scene s;
  auto view = dev::list_entities(s.world, s.tags, "")[0];
  view.pos = {100, 200};
  view.rotation = 45;
  view.velocity = Vec2{-7, 0};
  dev::apply(s.world, view);
  CHECK(s.world.position(s.bird).x == doctest::Approx(100));
  CHECK(s.world.rotation(s.bird) == doctest::Approx(45));
  CHECK(s.world.velocity(s.bird).x == doctest::Approx(-7));
  auto wall = dev::list_entities(s.world, s.tags, "wall")[0];
  wall.pos = {1, 2};
  dev::apply(s.world, wall);
  CHECK(s.world.position(s.wall).y == doctest::Approx(2));
}

TEST_CASE("applying a view of a destroyed entity does nothing") {
  Scene s;
  const auto view = dev::list_entities(s.world, s.tags, "")[0];
  s.world.destroy(s.bird);
  dev::apply(s.world, view);
  CHECK_FALSE(s.world.alive(s.bird));
}

TEST_CASE("collider shapes are outlines at the body, sensors in another color") {
  Scene s;
  TagSet none;
  s.world.spawn({none, {5, 5}, 0.0f, 0, {}, BodySpec{BodyType::Static, {Rect{{4, 4}}, true}, {}}, {}, {}});
  const auto shapes = dev::collider_shapes(s.world);
  REQUIRE(shapes.size() == 2);
  CHECK(std::holds_alternative<Circle>(shapes[0].shape.geometry));
  CHECK(shapes[0].pos.x == doctest::Approx(10));
  CHECK(shapes[0].shape.fill.a == 0);
  REQUIRE(shapes[0].shape.outline);
  REQUIRE(shapes[1].shape.outline);
  CHECK(*shapes[0].shape.outline != *shapes[1].shape.outline);
}
