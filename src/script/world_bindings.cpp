#include "script/world_bindings.hpp"

#include <array>
#include <cstring>
#include <vector>

#include "core/error.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/spec_parser.hpp"

namespace alpha::script {

ecs::World& world_of(ScriptState& s) {
  if (!s.world) throw core::Error("no active scene world");
  return *s.world;
}

ecs::EntityId id_arg(JSContext* ctx, JSValueConst v) {
  return static_cast<ecs::EntityId>(number_arg(ctx, v, "entity id"));
}

namespace {

ecs::World& world(ScriptState& s) { return world_of(s); }

// argv[1], argv[2] as a vector; `what` names it in errors.
core::Vec2 xy_args(JSContext* ctx, JSValueConst* argv, const std::string& what) {
  return {static_cast<float>(number_arg(ctx, argv[1], what + "[0]")),
          static_cast<float>(number_arg(ctx, argv[2], what + "[1]"))};
}

JSValue reset_world(JSContext*, ScriptState& s, JSValueConst*) {
  s.world = std::make_unique<ecs::World>(s.gravity, s.view_size, &s.random);
  return JS_UNDEFINED;
}

JSValue spawn(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewUint32(ctx, world(s).spawn(parse_entity_spec(ctx, argv[0], s)));
}

JSValue destroy(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  world(s).destroy(id_arg(ctx, argv[0]));
  return JS_UNDEFINED;
}

JSValue alive(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewBool(ctx, world(s).alive(id_arg(ctx, argv[0])));
}

JSValue has_tag(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const ecs::EntityId id = id_arg(ctx, argv[0]);
  const auto tag = s.tags.find(string_arg(ctx, argv[1], "tag"));
  return JS_NewBool(ctx, tag && world(s).alive(id) && world(s).tags(id).test(*tag));
}

JSValue tags(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const core::TagSet set = world(s).tags(id_arg(ctx, argv[0]));
  std::vector<std::string> names;
  for (std::size_t i = 0; i < core::kMaxTags; ++i) {
    if (set.test(i)) names.emplace_back(s.tags.name(static_cast<core::TagId>(i)));
  }
  return new_strings(ctx, names);
}

JSValue find_all(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const auto tag = s.tags.find(string_arg(ctx, argv[0], "tag"));
  if (!tag) return JS_NewArray(ctx);  // nothing ever spawned with it
  return new_ids(ctx, world(s).find_all(*tag));
}

JSValue position(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return new_vec2(ctx, world(s).position(id_arg(ctx, argv[0])));
}

JSValue set_position(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  world(s).set_position(id_arg(ctx, argv[0]), xy_args(ctx, argv, "pos"));
  return JS_UNDEFINED;
}

// positionsOf(tag): ArrayBuffer of 3n float64: n ids, then n x, then n y (oldest first).
JSValue positions_of(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const auto tag = s.tags.find(string_arg(ctx, argv[0], "positions(tag)"));
  const std::vector<ecs::EntityId> ids = tag ? world(s).find_all(*tag) : std::vector<ecs::EntityId>{};
  const std::size_t n = ids.size();
  std::vector<double> out(3 * n);
  for (std::size_t i = 0; i < n; ++i) {
    const core::Vec2 p = world(s).position(ids[i]);
    out[i] = ids[i];
    out[n + i] = p.x;
    out[2 * n + i] = p.y;
  }
  return JS_NewArrayBufferCopy(ctx, reinterpret_cast<const std::uint8_t*>(out.data()), out.size() * sizeof(double));
}

// setPositions(buffer): writes back a positionsOf buffer; entities destroyed since are skipped.
JSValue set_positions(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  std::size_t size = 0;
  const std::uint8_t* bytes = JS_GetArrayBuffer(ctx, &size, argv[0]);
  if (!bytes || size % (3 * sizeof(double)) != 0) {
    JS_FreeValue(ctx, JS_GetException(ctx));  // not an ArrayBuffer: our message instead
    throw core::Error("positions.write(): the buffer must come from ctx.positions(tag)");
  }
  const std::size_t n = size / (3 * sizeof(double));
  std::vector<double> v(3 * n);
  std::memcpy(v.data(), bytes, size);
  for (std::size_t i = 0; i < n; ++i) {
    if (!(v[i] >= 0 && v[i] <= 4294967295.0)) throw core::Error("positions.write(): ids must stay as read");
    const auto id = static_cast<ecs::EntityId>(v[i]);
    if (world(s).alive(id)) world(s).set_position(id, {static_cast<float>(v[n + i]), static_cast<float>(v[2 * n + i])});
  }
  return JS_UNDEFINED;
}

JSValue rotation(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewFloat64(ctx, world(s).rotation(id_arg(ctx, argv[0])));
}

JSValue set_rotation(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const auto degrees = static_cast<float>(number_arg(ctx, argv[1], "rotation"));
  world(s).set_rotation(id_arg(ctx, argv[0]), degrees);
  return JS_UNDEFINED;
}

JSValue z(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewInt32(ctx, world(s).z(id_arg(ctx, argv[0])));
}

JSValue set_z(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  world(s).set_z(id_arg(ctx, argv[0]), static_cast<int>(number_arg(ctx, argv[1], "z")));
  return JS_UNDEFINED;
}

JSValue velocity(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return new_vec2(ctx, world(s).velocity(id_arg(ctx, argv[0])));
}

JSValue set_velocity(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  world(s).set_velocity(id_arg(ctx, argv[0]), xy_args(ctx, argv, "body.velocity"));
  return JS_UNDEFINED;
}

JSValue pairs(JSContext* ctx, const std::vector<ecs::Contact>& contacts) {
  JSValue array = JS_NewArray(ctx);
  for (std::uint32_t i = 0; i < contacts.size(); ++i) {
    const std::array<std::uint32_t, 2> pair{contacts[i].a, contacts[i].b};
    JS_SetPropertyUint32(ctx, array, i, new_ids(ctx, pair));
  }
  return array;
}

// [[a, b, normalX, normalY, pointX, pointY, speed], ...]
JSValue hits(JSContext* ctx, const std::vector<physics::Hit>& list) {
  JSValue array = JS_NewArray(ctx);
  for (std::uint32_t i = 0; i < list.size(); ++i) {
    const physics::Hit& h = list[i];
    const std::array<double, 7> row{static_cast<double>(h.a), static_cast<double>(h.b), h.normal.x, h.normal.y, h.point.x, h.point.y, h.speed};
    JS_SetPropertyUint32(ctx, array, i, new_numbers(ctx, row));
  }
  return array;
}

JSValue step_world(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const ecs::StepEvents events = world(s).step(static_cast<float>(dt_arg(ctx, argv[0])));
  JSValue out = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, out, "hits", hits(ctx, events.hits));
  JS_SetPropertyStr(ctx, out, "hitEnds", pairs(ctx, events.hit_ends));
  JS_SetPropertyStr(ctx, out, "enters", pairs(ctx, events.enters));
  JS_SetPropertyStr(ctx, out, "exits", pairs(ctx, events.exits));
  JS_SetPropertyStr(ctx, out, "freed", new_ids(ctx, events.freed));
  const core::FrameTimes spent = world(s).take_times();
  for (std::size_t i = 0; i < core::kSections; ++i) s.times.ms[i] += spent.ms[i];
  return out;
}

constexpr std::array<Native, 18> kNatives{{
    {"positionsOf", guarded<positions_of>, 1}, {"setPositions", guarded<set_positions>, 1},
    {"resetWorld", guarded<reset_world>, 0},  {"spawn", guarded<spawn>, 1},
    {"destroy", guarded<destroy>, 1},         {"alive", guarded<alive>, 1},
    {"hasTag", guarded<has_tag>, 2},          {"tags", guarded<tags>, 1},
    {"findAll", guarded<find_all>, 1},        {"position", guarded<position>, 1},
    {"setPosition", guarded<set_position>, 3}, {"rotation", guarded<rotation>, 1},
    {"setRotation", guarded<set_rotation>, 2}, {"z", guarded<z>, 1},
    {"setZ", guarded<set_z>, 2},              {"velocity", guarded<velocity>, 1},
    {"setVelocity", guarded<set_velocity>, 3}, {"stepWorld", guarded<step_world>, 1},
}};

}  // namespace

void install_world_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
