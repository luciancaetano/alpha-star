#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/tags.hpp"
#include "ecs/world.hpp"

namespace alpha::ecs {

// One entity as tools see it (dev tools inspector, the agent control channel).
struct EntityView {
  EntityId id = 0;
  core::Vec2 pos;
  float rotation = 0.0f;  // degrees
  int z = 0;
  std::vector<std::string> tags;
  std::optional<core::Vec2> velocity;  // only entities with a body
  std::string animation{};             // playing sheet animation, "" for none
};

// Living entities, oldest first; with a filter, only those having a tag whose
// name contains it.
std::vector<EntityView> list_entities(const World& world, const core::TagRegistry& tags, std::string_view filter);

}  // namespace alpha::ecs
