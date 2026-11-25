#pragma once

#include <vector>

#include "ecs/entity_view.hpp"
#include "ecs/world.hpp"

namespace alpha::devtools {

// The inspector lists entities with ecs::list_entities and edits them here.
using ecs::EntityView;
using ecs::list_entities;

// Writes pos, rotation and (with a body) velocity back. Dead entities are skipped.
void apply(ecs::World& world, const EntityView& view);
// Physics view: one unfilled outline per collider (sensors in a second color).
std::vector<core::DrawItem> collider_shapes(const ecs::World& world);

}  // namespace alpha::devtools
