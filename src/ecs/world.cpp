#include "ecs/world.hpp"

#include <algorithm>
#include <string>
#include <tuple>

#include "core/error.hpp"
#include "ecs/components.hpp"

namespace alpha::ecs {

namespace {

EntityId to_id(entt::entity e) { return static_cast<EntityId>(entt::to_integral(e)); }

entt::entity to_entity(EntityId id) { return static_cast<entt::entity>(id); }

}  // namespace

World::World(core::Vec2 gravity) : physics_(gravity) {}

EntityId World::spawn(const core::EntitySpec& spec) {
  const entt::entity e = registry_.create();
  registry_.emplace<Transform>(e, spec.pos, spec.rotation, spec.z);
  registry_.emplace<Sequence>(e, next_sequence_++);
  registry_.emplace<Tags>(e, spec.tags);
  if (spec.shape) registry_.emplace<Visual>(e, *spec.shape);
  if (spec.body) {
    registry_.emplace<PhysicsBody>(e, physics_.create(to_id(e), *spec.body, spec.pos, spec.rotation));
    registry_.emplace<Collider>(e, spec.body->collider);
  }
  return to_id(e);
}

void World::destroy(EntityId id) {
  const entt::entity e = to_entity(id);
  if (registry_.valid(e)) registry_.emplace_or_replace<Doomed>(e);
}

bool World::alive(EntityId id) const {
  const entt::entity e = to_entity(id);
  return registry_.valid(e) && !registry_.all_of<Doomed>(e);
}

entt::entity World::living(EntityId id) const {
  if (!alive(id)) throw core::Error("entity " + std::to_string(id) + " is not alive");
  return to_entity(id);
}

std::vector<EntityId> World::find_all(core::TagId tag) const {
  std::vector<EntityId> ids;
  for (const EntityId id : entities()) {
    if (registry_.get<Tags>(to_entity(id)).set.test(tag)) ids.push_back(id);
  }
  return ids;
}

std::vector<EntityId> World::entities() const {
  std::vector<std::pair<std::uint64_t, EntityId>> found;
  for (auto [e, seq] : registry_.view<const Sequence>(entt::exclude<Doomed>).each()) {
    found.emplace_back(seq.value, to_id(e));
  }
  std::sort(found.begin(), found.end());
  std::vector<EntityId> ids;
  for (const auto& [seq, id] : found) ids.push_back(id);
  return ids;
}

std::optional<EntityId> World::find(core::TagId tag) const {
  const auto all = find_all(tag);
  if (all.empty()) return std::nullopt;
  return all.front();
}

core::TagSet World::tags(EntityId id) const { return registry_.get<Tags>(living(id)).set; }

core::Vec2 World::position(EntityId id) const { return registry_.get<Transform>(living(id)).pos; }

float World::rotation(EntityId id) const { return registry_.get<Transform>(living(id)).rotation; }

void World::set_position(EntityId id, core::Vec2 pos) { place(id, pos, rotation(id)); }

void World::set_rotation(EntityId id, float degrees) { place(id, position(id), degrees); }

void World::place(EntityId id, core::Vec2 pos, float degrees) {
  Transform& t = registry_.get<Transform>(living(id));
  t.pos = pos;
  t.rotation = degrees;
  if (has_body(id)) physics::PhysicsWorld::set_transform(body_of(id), pos, degrees);
}

int World::z(EntityId id) const { return registry_.get<Transform>(living(id)).z; }

void World::set_z(EntityId id, int z) { registry_.get<Transform>(living(id)).z = z; }

bool World::has_body(EntityId id) const { return registry_.all_of<PhysicsBody>(living(id)); }

b2BodyId World::body_of(EntityId id) const {
  if (!has_body(id)) {
    throw core::Error("entity " + std::to_string(id) + " has no body; spawn it with `body`");
  }
  return registry_.get<PhysicsBody>(to_entity(id)).id;
}

core::Vec2 World::velocity(EntityId id) const {
  return physics::PhysicsWorld::velocity(body_of(id));
}

void World::set_velocity(EntityId id, core::Vec2 velocity) {
  physics::PhysicsWorld::set_velocity(body_of(id), velocity);
}

void World::free_doomed() {
  const auto doomed = registry_.view<Doomed>();
  for (const entt::entity e : doomed) {
    if (const auto* body = registry_.try_get<PhysicsBody>(e)) physics_.destroy(body->id);
  }
  registry_.destroy(doomed.begin(), doomed.end());
}

void World::sync_bodies() {
  for (auto [e, body, t] : registry_.view<const PhysicsBody, Transform>().each()) {
    t.pos = physics::PhysicsWorld::position(body.id);
    t.rotation = physics::PhysicsWorld::rotation(body.id);
  }
}

std::vector<Contact> World::living_pairs(const std::vector<physics::Pair>& pairs) const {
  std::vector<Contact> out;
  for (const auto& p : pairs) {
    if (alive(p.a) && alive(p.b)) out.push_back({p.a, p.b});
  }
  return out;
}

StepEvents World::step(float dt) {
  free_doomed();
  const physics::StepContacts contacts = physics_.step(dt);
  sync_bodies();
  return {living_pairs(contacts.hits), living_pairs(contacts.sensor_enters)};
}

std::vector<core::DrawItem> World::draw_items() const {
  using Keyed = std::tuple<int, std::uint64_t, core::DrawItem>;
  std::vector<Keyed> keyed;
  const auto view = registry_.view<const Transform, const Visual, const Sequence>(entt::exclude<Doomed>);
  for (auto [e, t, v, seq] : view.each()) {
    keyed.emplace_back(t.z, seq.value, core::DrawItem{v.shape, t.pos, t.rotation});
  }
  std::sort(keyed.begin(), keyed.end(), [](const Keyed& a, const Keyed& b) {
    return std::tie(std::get<0>(a), std::get<1>(a)) < std::tie(std::get<0>(b), std::get<1>(b));
  });
  std::vector<core::DrawItem> items;
  for (auto& k : keyed) items.push_back(std::move(std::get<2>(k)));
  return items;
}

std::vector<PlacedCollider> World::colliders() const {
  std::vector<PlacedCollider> out;
  for (const EntityId id : entities()) {
    const entt::entity e = to_entity(id);
    if (const auto* c = registry_.try_get<Collider>(e)) {
      const Transform& t = registry_.get<Transform>(e);
      out.push_back({c->spec, t.pos, t.rotation});
    }
  }
  return out;
}

}  // namespace alpha::ecs
