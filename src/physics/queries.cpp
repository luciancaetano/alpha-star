#include "physics/queries.hpp"

#include <algorithm>
#include <array>
#include <tuple>

#include "physics/units.hpp"

namespace alpha::physics {

std::uint32_t owner_of(b2ShapeId shape) {
  const void* data = b2Body_GetUserData(b2Shape_GetBody(shape));
  return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(data));
}

namespace {

// Finds shapes of any category in `mask` (the query itself collides with all).
b2QueryFilter filter_of(std::uint64_t mask) { return {~std::uint64_t{0}, mask}; }

float collect_hit(b2ShapeId shape, b2Vec2 point, b2Vec2 normal, float fraction, void* context) {
  if (b2Shape_IsSensor(shape)) return -1.0f;  // skip, keep the ray going
  auto* hits = static_cast<std::vector<RayHit>*>(context);
  hits->push_back({owner_of(shape), to_pixels(point), {normal.x, normal.y}, fraction});
  return 1.0f;  // keep the full ray: every hit is collected
}

bool collect_owner(b2ShapeId shape, void* context) {
  static_cast<std::vector<std::uint32_t>*>(context)->push_back(owner_of(shape));
  return true;
}

std::vector<std::uint32_t> overlap(b2WorldId world, const b2ShapeProxy& proxy, std::uint64_t mask) {
  std::vector<std::uint32_t> owners;
  b2World_OverlapShape(world, &proxy, filter_of(mask), collect_owner, &owners);
  std::sort(owners.begin(), owners.end());
  owners.erase(std::unique(owners.begin(), owners.end()), owners.end());
  return owners;
}

}  // namespace

std::vector<RayHit> cast_ray(b2WorldId world, core::Vec2 from, core::Vec2 to, std::uint64_t mask) {
  std::vector<RayHit> hits;
  const b2Vec2 origin = to_meters(from);
  const b2Vec2 translation = b2Sub(to_meters(to), origin);
  if (b2LengthSquared(translation) == 0.0f) return hits;
  b2World_CastRay(world, origin, translation, filter_of(mask), collect_hit, &hits);
  std::sort(hits.begin(), hits.end(), [](const RayHit& a, const RayHit& b) {
    return std::tie(a.fraction, a.owner) < std::tie(b.fraction, b.owner);
  });
  return hits;
}

std::vector<std::uint32_t> overlap_circle(b2WorldId world, core::Vec2 center, float radius, std::uint64_t mask) {
  const b2Vec2 c = to_meters(center);
  return overlap(world, b2MakeProxy(&c, 1, radius / kPixelsPerMeter), mask);
}

std::vector<std::uint32_t> overlap_rect(b2WorldId world, core::Vec2 pos, core::Vec2 size, std::uint64_t mask) {
  const b2Vec2 a = to_meters(pos);
  const b2Vec2 b = to_meters(pos + size);
  const std::array<b2Vec2, 4> corners{{{a.x, a.y}, {b.x, a.y}, {b.x, b.y}, {a.x, b.y}}};
  return overlap(world, b2MakeProxy(corners.data(), 4, 0.0f), mask);
}

}  // namespace alpha::physics
