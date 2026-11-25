#pragma once

#include <box2d/box2d.h>

#include <cstdint>
#include <vector>

#include "core/vec2.hpp"

// World queries in engine units. `mask`: collision categories to consider
// (config.collision bits; all ones = every body). Results are deterministic.
namespace alpha::physics {

// `normal` points out of the hit surface; `fraction` 0..1 along from -> to.
struct RayHit {
  std::uint32_t owner = 0;
  core::Vec2 point;
  core::Vec2 normal;
  float fraction = 0.0f;
};

// The owner id a body was created with (PhysicsWorld::create).
std::uint32_t owner_of(b2ShapeId shape);

// Every solid collider the segment crosses (sensors are ignored), nearest
// first (ties: smaller owner first).
std::vector<RayHit> cast_ray(b2WorldId world, core::Vec2 from, core::Vec2 to, std::uint64_t mask);
// Owners of the colliders (sensors included) overlapping the circle / the
// axis-aligned rect at top-left `pos`, in ascending owner order, once each.
std::vector<std::uint32_t> overlap_circle(b2WorldId world, core::Vec2 center, float radius, std::uint64_t mask);
std::vector<std::uint32_t> overlap_rect(b2WorldId world, core::Vec2 pos, core::Vec2 size, std::uint64_t mask);

}  // namespace alpha::physics
