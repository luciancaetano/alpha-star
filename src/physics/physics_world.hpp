#pragma once

#include <box2d/box2d.h>

#include <cstdint>
#include <vector>

#include "core/entity_spec.hpp"
#include "core/vec2.hpp"
#include "physics/joints.hpp"

namespace alpha::physics {

// Owner ids pair up physics bodies with the entities that own them.
struct Pair {
  std::uint32_t a;
  std::uint32_t b;
};

// A solid contact that began: `normal` points from a to b, `point` is where
// they touch (pixels), `speed` how fast they approached along the normal (px/s).
struct Hit {
  std::uint32_t a = 0;
  std::uint32_t b = 0;
  core::Vec2 normal;
  core::Vec2 point;
  float speed = 0.0f;
};

struct StepContacts {
  std::vector<Hit> hits;            // solid contacts that began this step
  std::vector<Pair> hit_ends;       // solid contacts that ended (both bodies still exist)
  std::vector<Pair> sensor_enters;  // a = visitor, b = sensor
  std::vector<Pair> sensor_exits;   // a = visitor, b = sensor
};

// Box2D v3 world in engine units (pixels, degrees). Converts to meters inside.
// Forces, impulses and torques are in kg-pixel units: an impulse of
// mass * 100 changes the velocity by 100 px/s.
class PhysicsWorld {
 public:
  explicit PhysicsWorld(core::Vec2 gravity);
  ~PhysicsWorld();
  PhysicsWorld(const PhysicsWorld&) = delete;
  PhysicsWorld& operator=(const PhysicsWorld&) = delete;

  b2BodyId create(std::uint32_t owner, const core::BodySpec& spec, core::Vec2 pos,
                  float rotation);
  void destroy(b2BodyId body);
  StepContacts step(float dt);
  b2WorldId id() const { return world_; }
  Joints& joints() { return joints_; }
  const Joints& joints() const { return joints_; }

  static core::Vec2 position(b2BodyId body);
  static float rotation(b2BodyId body);
  static void set_transform(b2BodyId body, core::Vec2 pos, float rotation);
  static core::Vec2 velocity(b2BodyId body);
  static void set_velocity(b2BodyId body, core::Vec2 velocity);
  static float angular_velocity(b2BodyId body);  // degrees per second
  static void set_angular_velocity(b2BodyId body, float degrees_per_second);
  static float mass(b2BodyId body);  // kg (0 for static and kinematic bodies)
  static void apply_force(b2BodyId body, core::Vec2 force);  // this step, at the center
  static void apply_impulse(b2BodyId body, core::Vec2 impulse);
  static void apply_torque(b2BodyId body, float torque);

 private:
  b2WorldId world_;
  Joints joints_;
};

}  // namespace alpha::physics
