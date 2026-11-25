#pragma once

#include <entt/entity/registry.hpp>

#include <cstdint>
#include <optional>
#include <vector>

#include "core/entity_spec.hpp"
#include "physics/physics_world.hpp"

namespace alpha::ecs {

// Opaque handle given to scripts. Includes the EnTT version, so a stale
// handle to a recycled slot is detected as dead.
using EntityId = std::uint32_t;

struct Contact {
  EntityId a;
  EntityId b;
};

struct StepEvents {
  std::vector<Contact> hits;    // solid contacts that began
  std::vector<Contact> enters;  // a = visitor, b = sensor
};

// One scene's entities. Every accessor throws core::Error for a dead id.
class World {
 public:
  explicit World(core::Vec2 gravity);
  World(const World&) = delete;
  World& operator=(const World&) = delete;

  EntityId spawn(const core::EntitySpec& spec);
  // Immediately not alive; physics and storage are freed at the next step.
  void destroy(EntityId id);
  bool alive(EntityId id) const;

  // Oldest living entity with the tag.
  std::optional<EntityId> find(core::TagId tag) const;
  // All living entities with the tag, oldest first.
  std::vector<EntityId> find_all(core::TagId tag) const;
  core::TagSet tags(EntityId id) const;

  core::Vec2 position(EntityId id) const;
  void set_position(EntityId id, core::Vec2 pos);
  float rotation(EntityId id) const;
  void set_rotation(EntityId id, float degrees);
  int z(EntityId id) const;
  void set_z(EntityId id, int z);
  bool has_body(EntityId id) const;
  core::Vec2 velocity(EntityId id) const;
  void set_velocity(EntityId id, core::Vec2 velocity);

  // Frees destroyed entities, advances physics, syncs transforms.
  StepEvents step(float dt);
  // Living shapes, ordered by z then creation.
  std::vector<core::DrawItem> draw_items() const;

 private:
  entt::entity living(EntityId id) const;
  b2BodyId body_of(EntityId id) const;
  void place(EntityId id, core::Vec2 pos, float degrees);
  void free_doomed();
  void sync_bodies();
  std::vector<Contact> living_pairs(const std::vector<physics::Pair>& pairs) const;

  entt::registry registry_;
  physics::PhysicsWorld physics_;
  std::uint64_t next_sequence_ = 0;
};

}  // namespace alpha::ecs
