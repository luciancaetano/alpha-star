#pragma once

#include <box2d/box2d.h>

#include <cstdint>
#include <vector>

#include "core/entity_spec.hpp"
#include "core/vec2.hpp"

namespace alpha::physics {

// Owner ids pair up physics bodies with the entities that own them.
struct Pair {
  std::uint32_t a;
  std::uint32_t b;
};

struct StepContacts {
  std::vector<Pair> hits;           // solid contacts that began this step
  std::vector<Pair> sensor_enters;  // a = visitor, b = sensor
};

// Box2D v3 world in engine units (pixels, degrees). Converts to meters inside.
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

  static core::Vec2 position(b2BodyId body);
  static float rotation(b2BodyId body);
  static void set_transform(b2BodyId body, core::Vec2 pos, float rotation);
  static core::Vec2 velocity(b2BodyId body);
  static void set_velocity(b2BodyId body, core::Vec2 velocity);

 private:
  b2WorldId world_;
};

}  // namespace alpha::physics
