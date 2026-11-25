#pragma once

#include <box2d/box2d.h>

#include <cstdint>
#include <string>

#include "core/entity_spec.hpp"

// ECS components. Data only; World owns all logic.
namespace alpha::ecs {

struct Transform {
  core::Vec2 pos;
  float rotation = 0.0f;  // degrees
  int z = 0;
};

// Transform at the start of the current fixed step; rendering blends from it
// to Transform so motion is smooth at any display refresh rate.
struct Previous {
  core::Vec2 pos;
  float rotation = 0.0f;
};

// Creation order: makes find() and draw order deterministic despite id reuse.
struct Sequence {
  std::uint64_t value = 0;
};

struct Tags {
  core::TagSet set;
};

struct Visual {
  core::ShapeSpec shape;
};

// Plays a sheet animation into the entity's Visual (src) on every fixed step.
struct Animator {
  core::SheetPtr sheet;
  std::string name;
  float time = 0.0f;  // seconds since this animation started
};

struct PhysicsBody {
  b2BodyId id;
};

// The collider a body was made with (read back by the dev tools physics view).
struct Collider {
  core::ColliderSpec spec;
};

// Destroyed by script this step; removed at the start of the next step.
struct Doomed {};

}  // namespace alpha::ecs
