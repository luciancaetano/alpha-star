#pragma once

#include <box2d/box2d.h>

#include <cstdint>
#include <map>
#include <optional>

#include "core/vec2.hpp"

namespace alpha::physics {

// Rope: a distance joint that only stops stretching (slack below `length`).
enum class JointType { Revolute, Distance, Rope, Weld, Prismatic };

// lower..upper: degrees for revolute, pixels for prismatic (along the axis
// from the start) and distance (length range).
struct Limits {
  float lower = 0.0f;
  float upper = 0.0f;
};

// speed: degrees/s (revolute) or px/s (prismatic); max: the torque / force
// the motor may use, in the PhysicsWorld kg-pixel units.
struct Motor {
  float speed = 0.0f;
  float max = 0.0f;
};

// Anchors are world points in pixels at creation (revolute, weld and prismatic
// use anchor_a for both bodies). `axis`: prismatic slide direction.
struct JointSpec {
  JointType type = JointType::Revolute;
  core::Vec2 anchor_a;
  core::Vec2 anchor_b;
  core::Vec2 axis{1.0f, 0.0f};
  std::optional<float> length;  // distance, rope; default: the anchors' distance
  std::optional<Limits> limits;
  std::optional<Motor> motor;
  bool collide = false;  // the two bodies still collide with each other
};

// A world's joints by handle (1, 2, ...; never reused). A joint goes away with
// either body; its handle then reads as not alive.
class Joints {
 public:
  std::uint32_t create(b2WorldId world, b2BodyId a, b2BodyId b, const JointSpec& spec);
  void destroy(std::uint32_t handle);  // no-op for a gone joint
  bool alive(std::uint32_t handle) const;
  int count() const;  // living joints

 private:
  std::map<std::uint32_t, b2JointId> joints_;
  std::uint32_t next_ = 1;
};

}  // namespace alpha::physics
