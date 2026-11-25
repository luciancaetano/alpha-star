#pragma once

#include <entt/entity/registry.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/camera.hpp"
#include "core/entity_spec.hpp"
#include "core/particles.hpp"
#include "core/random.hpp"
#include "physics/physics_world.hpp"

namespace alpha::ecs {

// Opaque handle given to scripts. Includes the EnTT version, so a stale
// handle to a recycled slot is detected as dead.
using EntityId = std::uint32_t;

struct Contact {
  EntityId a;
  EntityId b;
};

// A body's collider where the body is now (dev tools physics view).
struct PlacedCollider {
  core::ColliderSpec collider;
  core::Vec2 pos;
  float rotation = 0.0f;  // degrees
};

struct StepEvents {
  std::vector<Contact> hits;    // solid contacts that began
  std::vector<Contact> enters;  // a = visitor, b = sensor
};

// One scene's entities. Every accessor throws core::Error for a dead id.
class World {
 public:
  // view_size: the screen in game pixels (the camera starts showing exactly it).
  // random: the game's seeded stream for particles (null: a private seed-0 one).
  explicit World(core::Vec2 gravity, core::Vec2 view_size = {}, core::Random* random = nullptr);
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
  // Every living entity, oldest first.
  std::vector<EntityId> entities() const;

  core::Vec2 position(EntityId id) const;
  void set_position(EntityId id, core::Vec2 pos);
  float rotation(EntityId id) const;
  void set_rotation(EntityId id, float degrees);
  int z(EntityId id) const;
  void set_z(EntityId id, int z);
  bool has_body(EntityId id) const;
  core::Vec2 velocity(EntityId id) const;
  void set_velocity(EntityId id, core::Vec2 velocity);

  // Sprite sheets. Name of the playing animation ("" without a sheet).
  std::string animation(EntityId id) const;
  // Switches to animation `name`; the same name keeps its time unless `restart`.
  // Throws listing the sheet's animations, or when the entity has no sheet.
  void play(EntityId id, const std::string& name, bool restart);
  std::pair<bool, bool> flip(EntityId id) const;
  void set_flip(EntityId id, bool x, bool y);

  // The entity's look: its shape, else its emitter's particle look (tint,
  // alpha, blend, text are edited here). Throws when it has neither.
  core::ShapeSpec& look(EntityId id);
  const core::ShapeSpec& look(EntityId id) const;
  const core::ShapeSpec* try_look(EntityId id) const;  // null when it has neither
  const core::Layer& layer(EntityId id) const;
  // Particles. Throws when the entity was not spawned with `emitter`.
  core::Emitter& emitter(EntityId id);
  const core::Emitter& emitter(EntityId id) const;
  bool has_emitter(EntityId id) const;
  // Live particles in the whole world.
  int particle_count() const;

  // Camera: moved on the step (follow, bounds, shake); `follow` a living entity or none.
  core::Camera& camera() { return camera_; }
  void follow(std::optional<EntityId> target) { target_ = target; }
  bool following() const { return target_.has_value(); }
  // The camera between the previous and the current step, like draw_items.
  core::View view(float blend = 1.0f) const;
  // The screen size the camera shows changed (scaling "expand").
  void resize_view(core::Vec2 size);

  // Frees destroyed entities, advances physics, syncs transforms, animations,
  // particle emitters and the camera.
  StepEvents step(float dt);
  // Living bodies' colliders, oldest first.
  std::vector<PlacedCollider> colliders() const;
  // Remembers every transform as "previous". Call at the start of each fixed step.
  void snapshot();
  // Living shapes and particles, ordered by layer z, z, then creation, placed
  // at the blend previous -> current by `blend` in [0, 1] (1 = current state).
  std::vector<core::DrawItem> draw_items(float blend = 1.0f) const;

 private:
  entt::entity living(EntityId id) const;
  b2BodyId body_of(EntityId id) const;
  void place(EntityId id, core::Vec2 pos, float degrees);
  void free_doomed();
  void sync_bodies();
  void animate(float dt);
  void step_view(float dt);
  core::ShapeSpec& visual(EntityId id);
  void attach(entt::entity e, const core::EntitySpec& spec);
  std::vector<Contact> living_pairs(const std::vector<physics::Pair>& pairs) const;

  entt::registry registry_;
  physics::PhysicsWorld physics_;
  std::uint64_t next_sequence_ = 0;
  core::Camera camera_;
  core::View previous_view_;
  std::optional<EntityId> target_;
  core::Random own_random_{0};
  core::Random* random_;
};

}  // namespace alpha::ecs
