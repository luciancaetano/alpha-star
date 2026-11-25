#include "script/physics_bindings.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "core/error.hpp"
#include "physics/queries.hpp"
#include "script/body_parser.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

namespace {

constexpr std::size_t kMaxCategories = 63;

using physics::PhysicsWorld;

float num(JSContext* ctx, JSValueConst v, const std::string& what) { return static_cast<float>(number_arg(ctx, v, what)); }

core::Vec2 xy(JSContext* ctx, JSValueConst* argv, int at, const std::string& what) {
  return {num(ctx, argv[at], what + "[0]"), num(ctx, argv[at + 1], what + "[1]")};
}

// What a query keeps: entities with any of `tags` (null = all) whose bodies
// are in `categories` (null = all).
struct Filter {
  std::vector<core::TagId> tags;
  bool any_tag = true;
  std::uint64_t mask = ~std::uint64_t{0};
};

Filter filter_args(JSContext* ctx, ScriptState& s, JSValueConst tags, JSValueConst categories, const std::string& what) {
  Filter f;
  if (!JS_IsNull(tags) && !JS_IsUndefined(tags)) {
    f.any_tag = false;
    for (const std::string& name : strings_arg(ctx, tags, what + " tags")) {
      if (const auto id = s.tags.find(name)) f.tags.push_back(*id);
    }
  }
  if (!JS_IsNull(categories) && !JS_IsUndefined(categories)) {
    f.mask = category_bits(s, strings_arg(ctx, categories, what + " categories"), what + " categories:");
  }
  return f;
}

bool keeps(const ecs::World& w, const Filter& f, ecs::EntityId id) {
  if (!w.alive(id)) return false;
  if (f.any_tag) return true;
  const core::TagSet set = w.tags(id);
  return std::any_of(f.tags.begin(), f.tags.end(), [&set](core::TagId t) { return set.test(t); });
}

void check_categories(const std::vector<std::string>& names) {
  if (names.size() > kMaxCategories) throw core::Error("config.collision.categories: at most 63 categories");
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (std::find(names.begin(), names.begin() + static_cast<std::ptrdiff_t>(i), names[i]) != names.begin() + static_cast<std::ptrdiff_t>(i)) {
      throw core::Error("config.collision.categories: \"" + names[i] + "\" is listed twice");
    }
  }
}

JSValue collision_categories(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  std::vector<std::string> names = strings_arg(ctx, argv[0], "config.collision.categories");
  check_categories(names);
  s.categories = std::move(names);
  return JS_UNDEFINED;
}

// raycast(fx, fy, tx, ty, tags, categories): [[id, px, py, nx, ny, fraction], ...] nearest first.
JSValue raycast(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  ecs::World& w = world_of(s);
  const Filter f = filter_args(ctx, s, argv[4], argv[5], "raycast");
  const auto hits = physics::cast_ray(w.physics().id(), xy(ctx, argv, 0, "raycast from"), xy(ctx, argv, 2, "raycast to"), f.mask);
  JSValue out = JS_NewArray(ctx);
  std::uint32_t n = 0;
  for (const physics::RayHit& h : hits) {
    if (!keeps(w, f, h.owner)) continue;
    const std::array<double, 6> row{static_cast<double>(h.owner), h.point.x, h.point.y, h.normal.x, h.normal.y, h.fraction};
    JS_SetPropertyUint32(ctx, out, n++, new_numbers(ctx, row));
  }
  return out;
}

// Owners in creation order that pass the filter.
JSValue kept_ids(JSContext* ctx, const ecs::World& w, const Filter& f, const std::vector<std::uint32_t>& owners) {
  std::vector<std::uint32_t> ids;
  for (const ecs::EntityId id : w.entities()) {
    if (std::binary_search(owners.begin(), owners.end(), id) && keeps(w, f, id)) ids.push_back(id);
  }
  return new_ids(ctx, ids);
}

// query(area, tags, categories): ids, oldest first, overlapping area = [cx, cy, r]
// (circle) or [x, y, w, h] (rect, top-left).
JSValue query(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const ecs::World& w = world_of(s);
  const std::vector<double> a = numbers_arg(ctx, argv[0], "query area");
  const bool circle = a.size() == 3;
  const std::string what = circle ? "queryCircle" : "queryRect";
  const Filter f = filter_args(ctx, s, argv[1], argv[2], what);
  const core::Vec2 at{static_cast<float>(a.at(0)), static_cast<float>(a.at(1))};
  const core::Vec2 size{static_cast<float>(a.at(2)), static_cast<float>(circle ? a[2] : a.at(3))};
  if (!(size.x > 0.0f && size.y > 0.0f)) throw core::Error(what + ": radius / width and height must be > 0");
  const b2WorldId id = w.physics().id();
  return kept_ids(ctx, w, f, circle ? physics::overlap_circle(id, at, size.x, f.mask) : physics::overlap_rect(id, at, size, f.mask));
}

b2BodyId body(JSContext* ctx, ScriptState& s, JSValueConst v) { return world_of(s).body(id_arg(ctx, v)); }

JSValue apply_force(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  PhysicsWorld::apply_force(body(ctx, s, argv[0]), xy(ctx, argv, 1, "body.applyForce"));
  return JS_UNDEFINED;
}

JSValue apply_impulse(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  PhysicsWorld::apply_impulse(body(ctx, s, argv[0]), xy(ctx, argv, 1, "body.applyImpulse"));
  return JS_UNDEFINED;
}

JSValue apply_torque(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  PhysicsWorld::apply_torque(body(ctx, s, argv[0]), num(ctx, argv[1], "body.applyTorque"));
  return JS_UNDEFINED;
}

// angularVelocity(id[, degreesPerSecond]): reads, or sets when given.
JSValue angular_velocity(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const b2BodyId b = body(ctx, s, argv[0]);
  if (!JS_IsUndefined(argv[1])) PhysicsWorld::set_angular_velocity(b, num(ctx, argv[1], "body.angularVelocity"));
  return JS_NewFloat64(ctx, PhysicsWorld::angular_velocity(b));
}

JSValue mass(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewFloat64(ctx, PhysicsWorld::mass(body(ctx, s, argv[0])));
}

// bodyType(id): "static" | "kinematic" | "dynamic", or null without a body.
JSValue body_type(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const ecs::World& w = world_of(s);
  const ecs::EntityId id = id_arg(ctx, argv[0]);
  if (!w.has_body(id)) return JS_NULL;
  const b2BodyType t = b2Body_GetType(w.body(id));
  return JS_NewString(ctx, t == b2_dynamicBody ? "dynamic" : (t == b2_kinematicBody ? "kinematic" : "static"));
}

constexpr std::array<Native, 9> kNatives{{
    {"collisionCategories", guarded<collision_categories>, 1},
    {"raycast", guarded<raycast>, 6},
    {"query", guarded<query>, 3},
    {"applyForce", guarded<apply_force>, 3},
    {"applyImpulse", guarded<apply_impulse>, 3},
    {"applyTorque", guarded<apply_torque>, 2},
    {"angularVelocity", guarded<angular_velocity>, 2},
    {"mass", guarded<mass>, 1},
    {"bodyType", guarded<body_type>, 1},
}};

}  // namespace

void install_physics_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
