#pragma once

#include <box2d/box2d.h>

#include <cstdint>
#include <string>

#include "core/entity_spec.hpp"
#include "core/particles.hpp"

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

// The draw layer the entity was spawned on (every entity has one).
struct Layered {
  core::Layer layer;
};

// A particle emitter riding on the entity (particles in world space).
struct Emission {
  core::Emitter emitter;
};

// A light riding on the entity (drawn into the lightmap).
struct Lit {
  core::LightSpec light;
};

struct PhysicsBody {
  b2BodyId id;
};

// The collider a body was made with (read back by the dev tools physics view).
struct Collider {
  core::ColliderSpec spec;
};

// A child entity: placed every step at its parent's transform composed with
// `local` (offset rotated by the parent) and `local_rotation`.
struct Parent {
  std::uint32_t id = 0;
  core::Vec2 local;
  float local_rotation = 0.0f;  // degrees
};

// The look as spawned, before entity.scale (added on the first scale change).
struct Scaled {
  core::Geometry base;
  float text_size = 0.0f;
  float scale = 1.0f;
};

// Game data of the entity as JSON text (entity.data, mirrored for tools).
struct Data {
  std::string json;
};

// Destroyed by script this step; removed at the start of the next step.
struct Doomed {};

}  // namespace alpha::ecs
