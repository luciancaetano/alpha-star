#pragma once

#include <entt/entity/registry.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include "ecs/components.hpp"

// Parent/child transforms. Pure over the registry; World owns the calls.
namespace alpha::ecs {

// Living entities passing keep(entity), oldest first (creation order makes
// iteration deterministic despite id reuse).
std::vector<entt::entity> oldest_first(const entt::registry& registry, const std::function<bool(entt::entity)>& keep);
// Entity ids (World::EntityId) of the entities, same order.
std::vector<std::uint32_t> ids_of(const std::vector<entt::entity>& entities);
// Where a child goes: its parent's transform composed with the link (pos, degrees).
std::pair<core::Vec2, float> placed(const Transform& parent, const Parent& link);
// Re-derives the link so the child sits at pos / degrees under `parent`.
void relink(Parent& link, const Transform& parent, core::Vec2 pos, float degrees);
// Living children of the entity with id `parent`, oldest first.
std::vector<entt::entity> children_of(const entt::registry& registry, std::uint32_t parent);
// Every living child, oldest first: a parent is always older than its children,
// so placing them in this order moves grandchildren after their parents.
std::vector<entt::entity> linked(const entt::registry& registry);

}  // namespace alpha::ecs
