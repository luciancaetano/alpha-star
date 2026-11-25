#include <doctest/doctest.h>

#include <cmath>

#include "physics/physics_world.hpp"
#include "physics/queries.hpp"

using namespace alpha::core;
using namespace alpha::physics;

namespace {

constexpr float kDt = 1.0f / 60.0f;
constexpr std::uint64_t kAll = ~std::uint64_t{0};

BodySpec square(BodyType type, float side, bool sensor = false) { return {type, {Rect{{side, side}}, sensor}, {}}; }

BodySpec ball() { return {BodyType::Dynamic, {Circle{10}}, {}}; }

float distance(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

void run(PhysicsWorld& world, int steps) {
  for (int i = 0; i < steps; ++i) world.step(kDt);
}

}  // namespace

TEST_CASE("a ray returns every solid collider it crosses, nearest first, skipping sensors") {
  PhysicsWorld world({0, 0});
  world.create(1, square(BodyType::Static, 20), {200, 0}, 0);
  world.create(2, square(BodyType::Static, 20), {100, 0}, 0);
  world.create(3, square(BodyType::Static, 20, true), {50, 0}, 0);
  const auto hits = cast_ray(world.id(), {0, 0}, {300, 0}, kAll);
  REQUIRE(hits.size() == 2);
  CHECK(hits[0].owner == 2);
  CHECK(hits[0].point.x == doctest::Approx(90));
  CHECK(hits[0].normal.x == doctest::Approx(-1));
  CHECK(hits[0].fraction == doctest::Approx(0.3));
  CHECK(hits[1].owner == 1);
  CHECK(cast_ray(world.id(), {0, 0}, {50, 0}, kAll).empty());
  CHECK(cast_ray(world.id(), {0, 0}, {0, 0}, kAll).empty());
}

TEST_CASE("a ray only sees the categories in its mask") {
  PhysicsWorld world({0, 0});
  BodySpec near = square(BodyType::Static, 20);
  near.filter.category = 2;
  world.create(1, near, {100, 0}, 0);
  world.create(2, square(BodyType::Static, 20), {200, 0}, 0);
  const auto hits = cast_ray(world.id(), {0, 0}, {300, 0}, 1);
  REQUIRE(hits.size() == 1);
  CHECK(hits[0].owner == 2);
}

TEST_CASE("circle and rect overlaps list owners once, ascending, sensors included") {
  PhysicsWorld world({0, 0});
  world.create(4, square(BodyType::Static, 20), {100, 100}, 0);
  world.create(2, square(BodyType::Static, 20, true), {130, 100}, 0);
  world.create(9, square(BodyType::Static, 20), {300, 300}, 0);
  CHECK(overlap_circle(world.id(), {115, 100}, 10, kAll) == std::vector<std::uint32_t>{2, 4});
  CHECK(overlap_circle(world.id(), {200, 200}, 10, kAll).empty());
  CHECK(overlap_rect(world.id(), {280, 280}, {10, 10}, kAll) == std::vector<std::uint32_t>{9});
  CHECK(overlap_rect(world.id(), {0, 0}, {400, 400}, 1).size() == 3);
}

TEST_CASE("a revolute joint pins two bodies at the anchor; destroying it frees them") {
  PhysicsWorld world({0, 900});
  const b2BodyId pivot = world.create(1, square(BodyType::Static, 10), {100, 100}, 0);
  const b2BodyId bob = world.create(2, ball(), {150, 100}, 0);
  const std::uint32_t j = world.joints().create(world.id(), pivot, bob, {JointType::Revolute, {100, 100}, {}, {1, 0}, {}, {}, {}, false});
  run(world, 60);
  CHECK(distance(PhysicsWorld::position(bob), {100, 100}) == doctest::Approx(50).epsilon(0.02));
  CHECK(world.joints().alive(j));
  CHECK(world.joints().count() == 1);
  world.joints().destroy(j);
  world.joints().destroy(j);  // no-op
  CHECK_FALSE(world.joints().alive(j));
  run(world, 60);
  CHECK(PhysicsWorld::position(bob).y > 200.0f);
}

TEST_CASE("distance keeps its length, a rope only its maximum") {
  PhysicsWorld world({0, 900});
  const b2BodyId top = world.create(1, square(BodyType::Static, 10), {100, 0}, 0);
  const b2BodyId a = world.create(2, ball(), {100, 60}, 0);
  const b2BodyId b = world.create(3, ball(), {300, 60}, 0);
  const b2BodyId anchor = world.create(4, square(BodyType::Static, 10), {300, 0}, 0);
  world.joints().create(world.id(), top, a, {JointType::Distance, {100, 0}, {100, 60}, {1, 0}, 40.0f, {}, {}, false});
  JointSpec rope{JointType::Rope, {300, 0}, {300, 60}, {1, 0}, 100.0f, {}, {}, false};
  world.joints().create(world.id(), anchor, b, rope);
  run(world, 120);
  CHECK(PhysicsWorld::position(a).y == doctest::Approx(40).epsilon(0.03));
  CHECK(PhysicsWorld::position(b).y == doctest::Approx(100).epsilon(0.03));
}

TEST_CASE("a joint goes away with its body") {
  PhysicsWorld world({0, 0});
  const b2BodyId a = world.create(1, ball(), {0, 0}, 0);
  const b2BodyId b = world.create(2, ball(), {50, 0}, 0);
  const std::uint32_t j = world.joints().create(world.id(), a, b, {JointType::Weld, {25, 0}, {}, {1, 0}, {}, {}, {}, false});
  world.destroy(b);
  CHECK_FALSE(world.joints().alive(j));
  CHECK(world.joints().count() == 0);
  world.joints().destroy(j);
}

TEST_CASE("a prismatic joint slides along its axis within limits, driven by its motor") {
  PhysicsWorld world({0, 0});
  const b2BodyId rail = world.create(1, square(BodyType::Static, 10), {0, 0}, 0);
  const b2BodyId car = world.create(2, ball(), {0, 0}, 0);
  JointSpec spec{JointType::Prismatic, {0, 0}, {}, {1, 0}, {}, Limits{0, 80}, Motor{200, 1000}, false};
  world.joints().create(world.id(), rail, car, spec);
  run(world, 120);
  CHECK(PhysicsWorld::position(car).x == doctest::Approx(80).epsilon(0.03));
  CHECK(PhysicsWorld::position(car).y == doctest::Approx(0).epsilon(0.03));
}

TEST_CASE("a revolute motor spins the body at its speed in degrees per second") {
  PhysicsWorld world({0, 0});
  const b2BodyId hub = world.create(1, square(BodyType::Static, 10), {0, 0}, 0);
  const b2BodyId wheel = world.create(2, ball(), {0, 0}, 0);
  JointSpec spec{JointType::Revolute, {0, 0}, {}, {1, 0}, {}, {}, Motor{90, 1e9f}, false};
  world.joints().create(world.id(), hub, wheel, spec);
  run(world, 30);
  CHECK(PhysicsWorld::angular_velocity(wheel) == doctest::Approx(90).epsilon(0.02));
}
