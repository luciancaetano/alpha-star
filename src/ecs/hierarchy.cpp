#include "ecs/hierarchy.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>
#include <utility>

#include "core/camera.hpp"
#include "core/error.hpp"
#include "ecs/world.hpp"

namespace alpha::ecs {

namespace {

// `entities` in creation order (their Sequence).
std::vector<entt::entity> by_creation(const entt::registry& registry, std::vector<entt::entity> entities) {
  std::sort(entities.begin(), entities.end(), [&registry](entt::entity a, entt::entity b) {
    return registry.get<Sequence>(a).value < registry.get<Sequence>(b).value;
  });
  return entities;
}

}  // namespace

std::vector<entt::entity> oldest_first(const entt::registry& registry, const std::function<bool(entt::entity)>& keep) {
  const auto all = registry.view<const Sequence>(entt::exclude<Doomed>);
  std::vector<entt::entity> kept;
  std::copy_if(all.begin(), all.end(), std::back_inserter(kept), keep);
  return by_creation(registry, std::move(kept));
}

std::pair<core::Vec2, float> placed(const Transform& parent, const Parent& link) {
  return {parent.pos + core::rotated(link.local, parent.rotation), parent.rotation + link.local_rotation};
}

void relink(Parent& link, const Transform& parent, core::Vec2 pos, float degrees) {
  link.local = core::rotated(pos - parent.pos, -parent.rotation);
  link.local_rotation = degrees - parent.rotation;
}

// Only linked entities are scanned: every set_position/destroy asks for
// children, and scanning (and sorting) the whole world made moving n entities
// O(n^2) per frame (bench: 2000 proxies cost 47 ms/step).
std::vector<entt::entity> children_of(const entt::registry& registry, std::uint32_t parent) {
  std::vector<entt::entity> children;
  for (auto [e, link] : registry.view<const Parent>(entt::exclude<Doomed>).each()) {
    if (link.id == parent) children.push_back(e);
  }
  return by_creation(registry, std::move(children));
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

void World::place(EntityId id, core::Vec2 pos, float degrees) {
  const entt::entity e = living(id);
  if (auto* link = registry_.try_get<Parent>(e)) relink(*link, registry_.get<Transform>(entity_of(link->id)), pos, degrees);
  put(e, pos, degrees);
  carry(e);
  const core::Vec2 jump = pos - registry_.get<Previous>(e).pos;
  if (std::hypot(jump.x, jump.y) > std::max(camera_.size.x, camera_.size.y) / 4) settle(e);  // a teleport
}

void World::settle(entt::entity e) {
  if (auto* p = registry_.try_get<Previous>(e)) {
    const Transform& t = registry_.get<Transform>(e);
    *p = {t.pos, t.rotation};
  }
  for (const entt::entity child : children_of(registry_, id_of(e))) settle(child);
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
