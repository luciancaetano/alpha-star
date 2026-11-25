#include "ecs/hierarchy.hpp"

#include <algorithm>
#include <string>

#include "core/camera.hpp"
#include "core/error.hpp"
#include "ecs/world.hpp"

namespace alpha::ecs {

std::vector<entt::entity> oldest_first(const entt::registry& registry, const std::function<bool(entt::entity)>& keep) {
  std::vector<std::pair<std::uint64_t, entt::entity>> found;
  for (auto [e, seq] : registry.view<const Sequence>(entt::exclude<Doomed>).each()) {
    if (keep(e)) found.emplace_back(seq.value, e);
  }
  std::sort(found.begin(), found.end());
  std::vector<entt::entity> out;
  out.reserve(found.size());
  for (const auto& p : found) out.push_back(p.second);
  return out;
}

std::pair<core::Vec2, float> placed(const Transform& parent, const Parent& link) {
  return {parent.pos + core::rotated(link.local, parent.rotation), parent.rotation + link.local_rotation};
}

void relink(Parent& link, const Transform& parent, core::Vec2 pos, float degrees) {
  link.local = core::rotated(pos - parent.pos, -parent.rotation);
  link.local_rotation = degrees - parent.rotation;
}

std::vector<entt::entity> children_of(const entt::registry& registry, std::uint32_t parent) {
  return oldest_first(registry, [&](entt::entity e) {
    const auto* link = registry.try_get<Parent>(e);
    return link && link->id == parent;
  });
}

std::vector<entt::entity> linked(const entt::registry& registry) {
  if (registry.view<Parent>().empty()) return {};  // the common case: no links at all
  return oldest_first(registry, [&](entt::entity e) { return registry.all_of<Parent>(e); });
}

// ---- World's parent/child and data members ----------------------------------

namespace {

EntityId id_of(entt::entity e) { return static_cast<EntityId>(entt::to_integral(e)); }

entt::entity entity_of(EntityId id) { return static_cast<entt::entity>(id); }

}  // namespace

void World::put(entt::entity e, core::Vec2 pos, float degrees) {
  Transform& t = registry_.get<Transform>(e);
  t.pos = pos;
  t.rotation = degrees;
  if (const auto* b = registry_.try_get<PhysicsBody>(e)) physics::PhysicsWorld::set_transform(b->id, pos, degrees);
}

void World::follow(entt::entity child) {
  const Parent& link = registry_.get<Parent>(child);
  const auto [pos, degrees] = placed(registry_.get<Transform>(entity_of(link.id)), link);
  put(child, pos, degrees);
}

void World::carry(entt::entity parent) {
  for (const entt::entity child : children_of(registry_, id_of(parent))) {
    follow(child);
    carry(child);
  }
}

void World::adopt(EntityId child, EntityId parent, core::Vec2 local) {
  living(parent);
  for (std::optional<EntityId> p = parent; p; p = this->parent(*p)) {
    if (*p == child) throw core::Error("entity " + std::to_string(child) + " cannot be its own ancestor");
  }
  registry_.emplace_or_replace<Parent>(living(child), parent, local, rotation(child));
  follow(entity_of(child));
  carry(entity_of(child));
}

std::optional<EntityId> World::parent(EntityId id) const {
  const auto* link = registry_.try_get<Parent>(living(id));
  return link ? std::optional<EntityId>(link->id) : std::nullopt;
}

std::vector<std::uint32_t> ids_of(const std::vector<entt::entity>& entities) {
  std::vector<std::uint32_t> ids;
  ids.reserve(entities.size());
  for (const entt::entity e : entities) ids.push_back(static_cast<std::uint32_t>(entt::to_integral(e)));
  return ids;
}

std::vector<EntityId> World::children(EntityId id) const { return ids_of(children_of(registry_, id_of(living(id)))); }

void World::follow_parents() {
  for (const entt::entity child : linked(registry_)) follow(child);
}

const std::string& World::data(EntityId id) const {
  static const std::string kNone;
  const auto* d = registry_.try_get<Data>(living(id));
  return d ? d->json : kNone;
}

void World::set_data(EntityId id, std::string json) { registry_.emplace_or_replace<Data>(living(id), std::move(json)); }

}  // namespace alpha::ecs
