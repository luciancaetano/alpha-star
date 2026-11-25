#include "devtools/inspector.hpp"

namespace alpha::devtools {

namespace {

constexpr core::Color kSolidOutline{0, 255, 120, 255};
constexpr core::Color kSensorOutline{255, 200, 0, 255};

}  // namespace

void apply(ecs::World& world, const EntityView& view) {
  if (!world.alive(view.id)) return;
  world.set_position(view.id, view.pos);
  world.set_rotation(view.id, view.rotation);
  if (view.velocity && world.has_body(view.id)) world.set_velocity(view.id, *view.velocity);
}

std::vector<core::DrawItem> collider_shapes(const ecs::World& world) {
  std::vector<core::DrawItem> out;
  for (const ecs::PlacedCollider& c : world.colliders()) {
    const core::Color outline = c.collider.sensor ? kSensorOutline : kSolidOutline;
    out.push_back({core::ShapeSpec{c.collider.geometry, core::Color{0, 0, 0, 0}, outline, {}}, c.pos, c.rotation});
  }
  return out;
}

}  // namespace alpha::devtools
