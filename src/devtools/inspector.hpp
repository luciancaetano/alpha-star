#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/tags.hpp"
#include "ecs/world.hpp"

namespace alpha::devtools {

// What the entity inspector shows and edits for one entity.
struct EntityView {
  ecs::EntityId id = 0;
  core::Vec2 pos;
  float rotation = 0.0f;  // degrees
  int z = 0;
  std::vector<std::string> tags;
  std::optional<core::Vec2> velocity;  // only entities with a body
};

// Living entities, oldest first; with a filter, only those having a tag whose
// name contains it.
std::vector<EntityView> list_entities(const ecs::World& world, const core::TagRegistry& tags,
                                      std::string_view filter);
// Writes pos, rotation and (with a body) velocity back. Dead entities are skipped.
void apply(ecs::World& world, const EntityView& view);
// Physics view: one unfilled outline per collider (sensors in a second color).
std::vector<core::DrawItem> collider_shapes(const ecs::World& world);

}  // namespace alpha::devtools
