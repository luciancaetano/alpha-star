#pragma once

#include <box2d/box2d.h>

#include <cstdint>

#include "core/entity_spec.hpp"

// ECS components. Data only; World owns all logic.
namespace alpha::ecs {

struct Transform {
  core::Vec2 pos;
  float rotation = 0.0f;  // degrees
  int z = 0;
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

struct PhysicsBody {
  b2BodyId id;
};

// Destroyed by script this step; removed at the start of the next step.
struct Doomed {};

}  // namespace alpha::ecs
