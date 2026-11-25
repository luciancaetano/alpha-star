#include "devtools/inspector.hpp"

#include <algorithm>

namespace alpha::devtools {

namespace {

constexpr core::Color kSolidOutline{0, 255, 120, 255};
constexpr core::Color kSensorOutline{255, 200, 0, 255};

std::vector<std::string> tag_names(core::TagSet set, const core::TagRegistry& tags) {
  std::vector<std::string> names;
  for (std::size_t i = 0; i < tags.size(); ++i) {
    if (set.test(i)) names.emplace_back(tags.name(static_cast<core::TagId>(i)));
  }
  return names;
}

bool matches(const std::vector<std::string>& names, std::string_view filter) {
  if (filter.empty()) return true;
  return std::any_of(names.begin(), names.end(),
                     [filter](const std::string& n) { return n.find(filter) != std::string::npos; });
}

EntityView view_of(const ecs::World& world, ecs::EntityId id, const core::TagRegistry& tags) {
  EntityView v{id, world.position(id), world.rotation(id), world.z(id), tag_names(world.tags(id), tags), {}};
  if (world.has_body(id)) v.velocity = world.velocity(id);
  return v;
}

}  // namespace

std::vector<EntityView> list_entities(const ecs::World& world, const core::TagRegistry& tags,
                                      std::string_view filter) {
  std::vector<EntityView> out;
  for (const ecs::EntityId id : world.entities()) {
    EntityView v = view_of(world, id, tags);
    if (matches(v.tags, filter)) out.push_back(std::move(v));
  }
  return out;
}

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
