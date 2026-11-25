#include "ecs/world.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "core/error.hpp"
#include "core/index_of.hpp"
#include "core/limits.hpp"
#include "ecs/components.hpp"
#include "ecs/effects.hpp"
#include "ecs/hierarchy.hpp"

namespace alpha::ecs {

namespace {

EntityId to_id(entt::entity e) { return static_cast<EntityId>(entt::to_integral(e)); }

entt::entity to_entity(EntityId id) { return static_cast<entt::entity>(id); }

// Puts the animation's current frame into the shape.
void show(const Animator& a, core::ShapeSpec& shape) {
  const core::Animation& anim = a.sheet->animations.at(a.name);
  shape.src = a.sheet->frame(anim.frames[core::frame_at(anim, a.time)]);
}

constexpr const char* kNoEmitter = "emitter; spawn it with `emitter: { ... }`";

// The entity's C component, or an error saying how to get one.
template <typename C, typename Registry>
auto& required(Registry& registry, entt::entity e, EntityId id, const char* missing) {
  auto* c = registry.template try_get<C>(e);
  if (!c) throw core::Error("entity " + std::to_string(id) + " has no " + missing);
  return *c;
}

}  // namespace

World::World(core::Vec2 gravity, core::Vec2 view_size, core::Random* random)
    : physics_(gravity),
      camera_(core::make_camera(view_size)),
      previous_view_(core::view_of(camera_)),
      random_(random ? random : &own_random_) {}

EntityId World::spawn(const core::EntitySpec& spec) {
  if (registry_.view<Transform>().size() >= core::kMaxEntities) {
    throw core::Error("spawn: a scene holds at most " + std::to_string(core::kMaxEntities) + " entities; destroy some first");
  }
  const entt::entity e = registry_.create();
  registry_.emplace<Transform>(e, spec.pos, spec.rotation, spec.z);
  registry_.emplace<Previous>(e, spec.pos, spec.rotation);
  registry_.emplace<Sequence>(e, next_sequence_++);
  registry_.emplace<Tags>(e, spec.tags);
  registry_.emplace<Layered>(e, spec.layer);
  attach(e, spec);
  return to_id(e);
}

// The optional parts of a spawn: shape, body, emitter, animation.
void World::attach(entt::entity e, const core::EntitySpec& spec) {
  if (spec.shape) registry_.emplace<Visual>(e, *spec.shape);
  if (spec.emitter) registry_.emplace<Emission>(e, core::Emitter(*spec.emitter));
  if (spec.light) registry_.emplace<Lit>(e, *spec.light);
  if (spec.body) {
    registry_.emplace<PhysicsBody>(e, physics_.create(to_id(e), *spec.body, spec.pos, spec.rotation));
    registry_.emplace<Collider>(e, spec.body->collider);
  }
  if (spec.shape && spec.shape->animation) {
    const core::AnimationSpec& anim = *spec.shape->animation;
    registry_.emplace<Animator>(e, anim.sheet, "", 0.0f);
    if (!anim.name.empty()) play(to_id(e), anim.name, true);
  }
}

void World::destroy(EntityId id) {
  if (!alive(id)) return;
  for (const entt::entity child : children_of(registry_, id)) destroy(to_id(child));
  registry_.emplace<Doomed>(to_entity(id));
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
  return ids_of(oldest_first(registry_, [](entt::entity) { return true; }));
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
  const entt::entity e = living(id);
  if (auto* link = registry_.try_get<Parent>(e)) relink(*link, registry_.get<Transform>(to_entity(link->id)), pos, degrees);
  put(e, pos, degrees);
  carry(e);
}

int World::z(EntityId id) const { return registry_.get<Transform>(living(id)).z; }

void World::set_z(EntityId id, int z) { registry_.get<Transform>(living(id)).z = z; }

bool World::has_body(EntityId id) const { return registry_.all_of<PhysicsBody>(living(id)); }

b2BodyId World::body(EntityId id) const {
  if (!has_body(id)) {
    throw core::Error("entity " + std::to_string(id) + " has no body; spawn it with `body`");
  }
  return registry_.get<PhysicsBody>(to_entity(id)).id;
}

core::Vec2 World::velocity(EntityId id) const { return physics::PhysicsWorld::velocity(body(id)); }

void World::set_velocity(EntityId id, core::Vec2 velocity) { physics::PhysicsWorld::set_velocity(body(id), velocity); }

std::vector<EntityId> World::free_doomed() {
  const auto doomed = registry_.view<Doomed>();
  std::vector<EntityId> freed;
  for (const entt::entity e : doomed) {
    if (const auto* body = registry_.try_get<PhysicsBody>(e)) physics_.destroy(body->id);
    freed.push_back(to_id(e));
  }
  registry_.destroy(doomed.begin(), doomed.end());
  return freed;
}

void World::sync_bodies() {
  for (auto [e, body, t] : registry_.view<const PhysicsBody, Transform>().each()) {
    t.pos = physics::PhysicsWorld::position(body.id);
    t.rotation = physics::PhysicsWorld::rotation(body.id);
  }
}

template <typename T>
std::vector<T> World::living_only(const std::vector<T>& pairs) const {
  std::vector<T> out;
  for (const T& p : pairs) {
    if (alive(p.a) && alive(p.b)) out.push_back(p);
  }
  return out;
}

core::ShapeSpec& World::visual(EntityId id) {
  return required<Visual>(registry_, living(id), id, "shape; spawn it with `shape`").shape;
}

core::ShapeSpec& World::look(EntityId id) {
  return const_cast<core::ShapeSpec&>(std::as_const(*this).look(id));
}

const core::ShapeSpec* World::try_look(EntityId id) const {
  const entt::entity e = living(id);
  if (const auto* v = registry_.try_get<Visual>(e)) return &v->shape;
  if (const auto* em = registry_.try_get<Emission>(e)) return &em->emitter.spec().look;
  return nullptr;
}

const core::ShapeSpec& World::look(EntityId id) const {
  if (const core::ShapeSpec* shape = try_look(id)) return *shape;
  throw core::Error("entity " + std::to_string(id) + " has no shape or emitter; spawn it with `shape` or `emitter`");
}

const core::Layer& World::layer(EntityId id) const { return registry_.get<Layered>(living(id)).layer; }

bool World::has_emitter(EntityId id) const { return registry_.all_of<Emission>(living(id)); }

core::Emitter& World::emitter(EntityId id) {
  return required<Emission>(registry_, living(id), id, kNoEmitter).emitter;
}

const core::Emitter& World::emitter(EntityId id) const {
  return required<Emission>(registry_, living(id), id, kNoEmitter).emitter;
}

int World::particle_count() const { return ecs::particle_count(registry_); }

std::string World::animation(EntityId id) const {
  const auto* a = registry_.try_get<Animator>(living(id));
  return a ? a->name : std::string();
}

void World::play(EntityId id, const std::string& name, bool restart) {
  auto* a = registry_.try_get<Animator>(living(id));
  if (!a) {
    throw core::Error("entity " + std::to_string(id) + " has no sprite sheet; spawn it with shape { sheet, animation }");
  }
  if (!a->sheet->animations.contains(name)) {
    throw core::Error("animation \"" + name + "\" is not in the entity's sheet; animations: " + core::keys_of(a->sheet->animations, ", "));
  }
  if (restart || a->name != name) *a = {a->sheet, name, 0.0f};
  show(*a, visual(id));
}

float World::scale(EntityId id) const {
  const auto* s = registry_.try_get<Scaled>(living(id));
  return s ? s->scale : 1.0f;
}

void World::set_scale(EntityId id, float scale) {
  core::ShapeSpec& shape = visual(id);
  const float text_size = shape.text ? shape.text->size : 0.0f;
  rescale(shape, registry_.get_or_emplace<Scaled>(to_entity(id), shape.geometry, text_size, 1.0f), scale);
}

std::pair<bool, bool> World::flip(EntityId id) const {
  const auto* v = registry_.try_get<Visual>(living(id));
  return v ? std::pair{v->shape.flip_x, v->shape.flip_y} : std::pair{false, false};
}

void World::set_flip(EntityId id, bool x, bool y) {
  core::ShapeSpec& shape = visual(id);
  shape.flip_x = x;
  shape.flip_y = y;
}

void World::animate(float dt) {
  for (auto [e, a, v] : registry_.view<Animator, Visual>().each()) {
    a.time += dt;
    if (!a.name.empty()) show(a, v.shape);  // a sheet sprite without animation keeps frame 0
  }
}

void World::step_view(float dt) {
  std::optional<core::Vec2> target;
  if (target_ && alive(*target_)) target = position(*target_);
  else target_.reset();
  core::step_camera(camera_, target, dt);
}

core::View World::view(float blend) const { return core::blend(previous_view_, core::view_of(camera_), blend); }

void World::resize_view(core::Vec2 size) {
  camera_.size = size;
  previous_view_.size = size;
}

StepEvents World::step(float dt) {
  std::vector<EntityId> freed = free_doomed();
  physics::StepContacts contacts;
  core::timed(times_[core::Section::physics], [&] { contacts = physics_.step(dt); sync_bodies(); });
  follow_parents();
  core::timed(times_[core::Section::animation], [&] { animate(dt); });
  core::timed(times_[core::Section::particles], [&] { step_emitters(registry_, dt, *random_); });
  step_view(dt);
  return {living_only(contacts.hits), living_only(contacts.hit_ends), living_only(contacts.sensor_enters),
          living_only(contacts.sensor_exits), std::move(freed)};
}

core::FrameTimes World::take_times() { return std::exchange(times_, {}); }

void World::snapshot() {
  for (auto [e, t, p] : registry_.view<const Transform, Previous>().each()) p = {t.pos, t.rotation};
  previous_view_ = core::view_of(camera_);
}

std::vector<core::DrawItem> World::draw_items(float blend) const { return draw_list(registry_, blend); }
std::vector<core::LightItem> World::lights(float blend) const { return light_list(registry_, blend); }

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
