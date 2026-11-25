#include <doctest/doctest.h>

#include "physics/physics_world.hpp"

using namespace alpha::core;
using alpha::physics::PhysicsWorld;
using alpha::physics::StepContacts;

namespace {

constexpr float kDt = 1.0f / 60.0f;

BodySpec box(BodyType type, Vec2 size, bool sensor = false) {
  return {type, {Rect{size}, sensor}, {}};
}

template <typename Pred>
StepContacts step_until(PhysicsWorld& world, int max_steps, Pred done) {
  for (int i = 0; i < max_steps; ++i) {
    StepContacts c = world.step(kDt);
    if (done(c)) return c;
  }
  return {};
}

}  // namespace

TEST_CASE("dynamic body falls under gravity in pixels") {
  PhysicsWorld world({0, 900});
  const b2BodyId ball = world.create(1, {BodyType::Dynamic, {Circle{10}, false}, {}}, {50, 50}, 0);
  for (int i = 0; i < 60; ++i) world.step(kDt);
  CHECK(PhysicsWorld::position(ball).y == doctest::Approx(50 + 450).epsilon(0.05));
  CHECK(PhysicsWorld::velocity(ball).y == doctest::Approx(900).epsilon(0.05));
}

TEST_CASE("falling body reports a hit with the ground's owner") {
  PhysicsWorld world({0, 900});
  world.create(7, box(BodyType::Static, {200, 20}), {100, 200}, 0);
  world.create(3, {BodyType::Dynamic, {Circle{10}, false}, {}}, {100, 100}, 0);
  const auto c = step_until(world, 120, [](const StepContacts& s) { return !s.hits.empty(); });
  REQUIRE(c.hits.size() == 1);
  const auto a = c.hits[0].a;
  const auto b = c.hits[0].b;
  CHECK(((a == 3 && b == 7) || (a == 7 && b == 3)));
}

TEST_CASE("moving kinematic sensor reports visitor then sensor") {
  PhysicsWorld world({0, 0});
  world.create(5, {BodyType::Kinematic, {Rect{{4, 100}}, true}, {-300, 0}}, {200, 100}, 0);
  world.create(2, {BodyType::Dynamic, {Circle{10}, false}, {}}, {100, 100}, 0);
  const auto c =
      step_until(world, 60, [](const StepContacts& s) { return !s.sensor_enters.empty(); });
  REQUIRE(c.sensor_enters.size() == 1);
  CHECK(c.sensor_enters[0].a == 2);
  CHECK(c.sensor_enters[0].b == 5);
}

TEST_CASE("transform and velocity setters round-trip in engine units") {
  PhysicsWorld world({0, 0});
  const b2BodyId body = world.create(1, box(BodyType::Kinematic, {10, 10}), {0, 0}, 0);
  PhysicsWorld::set_transform(body, {120, 80}, 30);
  PhysicsWorld::set_velocity(body, {-120, 0});
  CHECK(PhysicsWorld::position(body).x == doctest::Approx(120));
  CHECK(PhysicsWorld::position(body).y == doctest::Approx(80));
  CHECK(PhysicsWorld::rotation(body) == doctest::Approx(30).epsilon(0.005));  // b2MakeRot is approximate
  CHECK(PhysicsWorld::velocity(body).x == doctest::Approx(-120));
  world.destroy(body);
  CHECK_FALSE(b2Body_IsValid(body));
}
