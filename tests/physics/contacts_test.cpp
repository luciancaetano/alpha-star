#include <doctest/doctest.h>

#include <cmath>

#include "physics/physics_world.hpp"

using namespace alpha::core;
using alpha::physics::Hit;
using alpha::physics::PhysicsWorld;
using alpha::physics::StepContacts;

namespace {

constexpr float kDt = 1.0f / 60.0f;

BodySpec ball(float radius = 10) { return {BodyType::Dynamic, {Circle{radius}}, {}}; }

BodySpec ground(Vec2 size = {400, 20}) { return {BodyType::Static, {Rect{size}}, {}}; }

template <typename Pred>
StepContacts step_until(PhysicsWorld& world, int max_steps, Pred done) {
  for (int i = 0; i < max_steps; ++i) {
    StepContacts c = world.step(kDt);
    if (done(c)) return c;
  }
  return {};
}

bool any_hit(const StepContacts& s) { return !s.hits.empty(); }

// Steps until the ball rests or `steps` pass; returns its y.
float settle(PhysicsWorld& world, b2BodyId body, int steps = 120) {
  for (int i = 0; i < steps; ++i) world.step(kDt);
  return PhysicsWorld::position(body).y;
}

}  // namespace

TEST_CASE("a hit carries the normal from a to b, the contact point and the approach speed") {
  PhysicsWorld world({0, 900});
  world.create(7, ground(), {100, 200}, 0);
  world.create(3, ball(), {100, 100}, 0);
  const StepContacts c = step_until(world, 120, any_hit);
  REQUIRE(c.hits.size() == 1);
  const Hit h = c.hits[0];
  const float down = h.a == 3 ? 1.0f : -1.0f;  // from the ball to the ground is down (+y)
  CHECK(h.normal.y * down == doctest::Approx(1.0f));
  CHECK(h.normal.x == doctest::Approx(0.0f));
  CHECK(h.point.x == doctest::Approx(100).epsilon(0.02));
  CHECK(h.point.y == doctest::Approx(190).epsilon(0.02));
  CHECK(h.speed > 300.0f);  // fell ~80 px: about sqrt(2 * 900 * 80) = 380 px/s
  CHECK(h.speed < 450.0f);
}

TEST_CASE("solid contacts that stop touching report a hit end") {
  PhysicsWorld world({0, 900});
  world.create(7, ground(), {100, 200}, 0);
  const b2BodyId b = world.create(3, ball(), {100, 100}, 0);
  step_until(world, 120, any_hit);
  PhysicsWorld::set_transform(b, {100, 20}, 0);
  const StepContacts c = world.step(kDt);
  REQUIRE(c.hit_ends.size() == 1);
  CHECK(c.hit_ends[0].a + c.hit_ends[0].b == 10);
}

TEST_CASE("a visitor leaving a sensor reports an exit, visitor first") {
  PhysicsWorld world({0, 0});
  world.create(5, {BodyType::Kinematic, {Rect{{4, 100}}, true}, {-600, 0}}, {200, 100}, 0);
  world.create(2, ball(), {100, 100}, 0);
  const auto c = step_until(world, 60, [](const StepContacts& s) { return !s.sensor_exits.empty(); });
  REQUIRE(c.sensor_exits.size() == 1);
  CHECK(c.sensor_exits[0].a == 2);
  CHECK(c.sensor_exits[0].b == 5);
}

TEST_CASE("a one-way platform holds what lands on it and lets what rises pass") {
  PhysicsWorld world({0, 900});
  BodySpec ledge = ground({200, 10});
  ledge.collider.one_way = true;
  world.create(1, ledge, {100, 200}, 0);
  const b2BodyId lander = world.create(2, ball(), {100, 150}, 0);
  CHECK(settle(world, lander) == doctest::Approx(195 - 10).epsilon(0.02));

  BodySpec riser = ball();
  riser.velocity = {0, -600};
  const b2BodyId up = world.create(3, riser, {160, 260}, 0);
  for (int i = 0; i < 20; ++i) world.step(kDt);
  CHECK(PhysicsWorld::position(up).y < 180.0f);  // passed up through the ledge
}

TEST_CASE("fixed rotation ignores torque; spinning bodies report degrees per second") {
  PhysicsWorld world({0, 0});
  BodySpec fixed = ball();
  fixed.fixed_rotation = true;
  const b2BodyId a = world.create(1, fixed, {0, 0}, 0);
  const b2BodyId b = world.create(2, ball(), {100, 0}, 0);
  PhysicsWorld::apply_torque(a, 1e5f);
  PhysicsWorld::apply_torque(b, 1e5f);
  world.step(kDt);
  CHECK(PhysicsWorld::angular_velocity(a) == doctest::Approx(0.0f));
  CHECK(PhysicsWorld::angular_velocity(b) > 1.0f);
  PhysicsWorld::set_angular_velocity(b, 90);
  CHECK(PhysicsWorld::angular_velocity(b) == doctest::Approx(90));
}

TEST_CASE("gravity scale, bullet flag and density reach the body") {
  PhysicsWorld world({0, 900});
  BodySpec floating = ball();
  floating.gravity_scale = 0.0f;
  floating.bullet = true;
  floating.collider.material.density = 4.0f;
  const b2BodyId f = world.create(1, floating, {0, 0}, 0);
  const b2BodyId normal = world.create(2, ball(), {100, 0}, 0);
  CHECK(settle(world, f, 30) == doctest::Approx(0.0f));
  CHECK(b2Body_IsBullet(f));
  CHECK(PhysicsWorld::mass(f) == doctest::Approx(4 * PhysicsWorld::mass(normal)));
}

TEST_CASE("an impulse of mass * dv changes the velocity by dv px/s; a force accelerates") {
  PhysicsWorld world({0, 0});
  const b2BodyId b = world.create(1, ball(), {0, 0}, 0);
  PhysicsWorld::apply_impulse(b, Vec2{100, 0} * PhysicsWorld::mass(b));
  CHECK(PhysicsWorld::velocity(b).x == doctest::Approx(100));
  PhysicsWorld::apply_force(b, Vec2{0, 600} * PhysicsWorld::mass(b));
  world.step(kDt);
  CHECK(PhysicsWorld::velocity(b).y == doctest::Approx(10).epsilon(0.01));  // 600 px/s^2 for 1/60 s
}

TEST_CASE("restitution 1 bounces back up; the default material does not") {
  PhysicsWorld world({0, 900});
  world.create(9, ground({800, 20}), {300, 200}, 0);
  BodySpec bouncy = ball();
  bouncy.collider.material.restitution = 1.0f;
  const b2BodyId b = world.create(1, bouncy, {100, 100}, 0);
  const b2BodyId dull = world.create(2, ball(), {300, 100}, 0);
  for (int i = 0; i < 40; ++i) world.step(kDt);  // both hit the ground at ~0.4 s
  CHECK(PhysicsWorld::velocity(b).y < -150.0f);
  CHECK(std::abs(PhysicsWorld::velocity(dull).y) < 20.0f);
}

TEST_CASE("bodies whose categories are not in each other's mask pass through") {
  PhysicsWorld world({0, 900});
  BodySpec floor = ground();
  floor.filter = {2, ~std::uint64_t{0}};
  world.create(9, floor, {100, 200}, 0);
  BodySpec ghost = ball();
  ghost.filter = {4, ~std::uint64_t{2}};  // collides with everything but category 2
  const b2BodyId g = world.create(1, ghost, {100, 100}, 0);
  CHECK(settle(world, g, 60) > 300.0f);
}
