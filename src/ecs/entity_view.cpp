#include "ecs/entity_view.hpp"

#include <algorithm>

namespace alpha::ecs {

namespace {

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

EntityView view_of(const World& world, EntityId id, const core::TagRegistry& tags) {
  EntityView v;
  v.id = id;
  v.pos = world.position(id);
  v.rotation = world.rotation(id);
  v.z = world.z(id);
  v.tags = tag_names(world.tags(id), tags);
  if (world.has_body(id)) v.velocity = world.velocity(id);
  v.animation = world.animation(id);
  v.layer = world.layer(id).name;
  if (const core::ShapeSpec* look = world.try_look(id)) {
    v.alpha = look->alpha;
    if (look->text) v.text = look->text->text;
  }
  v.particles = world.has_emitter(id) ? world.emitter(id).count() : 0;
  v.data = world.data(id);
  return v;
}

}  // namespace

std::vector<EntityView> list_entities(const World& world, const core::TagRegistry& tags, std::string_view filter) {
  std::vector<EntityView> out;
  for (const EntityId id : world.entities()) {
    EntityView v = view_of(world, id, tags);
    if (matches(v.tags, filter)) out.push_back(std::move(v));
  }
  return out;
}

}  // namespace alpha::ecs
