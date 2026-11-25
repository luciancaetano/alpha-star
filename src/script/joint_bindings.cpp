#include "script/joint_bindings.hpp"

#include <array>
#include <string>
#include <utility>

#include "core/error.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/spec_fields.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

namespace {

using physics::JointSpec;
using physics::JointType;

constexpr std::array<std::pair<const char*, JointType>, 5> kTypes{{{"revolute", JointType::Revolute},
                                                                  {"distance", JointType::Distance},
                                                                  {"rope", JointType::Rope},
                                                                  {"weld", JointType::Weld},
                                                                  {"prismatic", JointType::Prismatic}}};

const std::string kJoint = "joint: ";

core::Vec2 point(JSContext* ctx, JSValueConst obj, const char* name, core::Vec2 fallback) {
  if (!has(ctx, obj, name)) return fallback;
  return vec2_arg(ctx, field(ctx, obj, name).get(), kJoint + name);
}

// anchors: [[ax, ay], [bx, by]] (default: the two bodies' positions).
void anchors(JSContext* ctx, JSValueConst opts, JointSpec& spec, core::Vec2 pa, core::Vec2 pb) {
  spec.anchor_a = pa;
  spec.anchor_b = pb;
  if (!has(ctx, opts, "anchors")) return;
  const Value list = field(ctx, opts, "anchors");
  spec.anchor_a = vec2_arg(ctx, list.at(0).get(), kJoint + "anchors[0]");
  spec.anchor_b = vec2_arg(ctx, list.at(1).get(), kJoint + "anchors[1]");
}

void limits_and_motor(JSContext* ctx, JSValueConst opts, JointSpec& spec) {
  if (has(ctx, opts, "limits")) {
    const core::Vec2 l = vec2_arg(ctx, field(ctx, opts, "limits").get(), kJoint + "limits");
    if (l.x > l.y) throw core::Error(kJoint + "limits must be [lower, upper] with lower <= upper");
    spec.limits = physics::Limits{l.x, l.y};
  }
  if (has(ctx, opts, "motor")) {
    const Value m = field(ctx, opts, "motor");
    if (!JS_IsObject(m.get())) throw core::Error(kJoint + "motor must be { speed, max }");
    spec.motor = physics::Motor{static_cast<float>(number_arg(ctx, m.prop("speed").get(), kJoint + "motor.speed")),
                                static_cast<float>(number_arg(ctx, m.prop("max").get(), kJoint + "motor.max"))};
  }
}

JointSpec joint_spec(JSContext* ctx, JSValueConst opts, core::Vec2 pa, core::Vec2 pb) {
  if (!JS_IsObject(opts)) throw core::Error(kJoint + "expected { type, a, b, ... }");
  JointSpec spec;
  spec.type = choice(ctx, field(ctx, opts, "type").get(), "joint type", kTypes);
  anchors(ctx, opts, spec, pa, pb);
  if (spec.type != JointType::Distance && spec.type != JointType::Rope) spec.anchor_a = point(ctx, opts, "anchor", pb);
  spec.axis = point(ctx, opts, "axis", spec.axis);
  if (has(ctx, opts, "length")) {
    spec.length = static_cast<float>(number_arg(ctx, field(ctx, opts, "length").get(), kJoint + "length"));
  }
  limits_and_motor(ctx, opts, spec);
  spec.collide = JS_ToBool(ctx, field(ctx, opts, "collide").get()) == 1;
  return spec;
}

// joint(a, b, opts): handle.
JSValue joint(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  ecs::World& w = world_of(s);
  const ecs::EntityId a = id_arg(ctx, argv[0]);
  const ecs::EntityId b = id_arg(ctx, argv[1]);
  if (a == b) throw core::Error(kJoint + "a and b must be two different entities");
  const JointSpec spec = joint_spec(ctx, argv[2], w.position(a), w.position(b));
  return JS_NewUint32(ctx, w.physics().joints().create(w.physics().id(), w.body(a), w.body(b), spec));
}

std::uint32_t handle(JSContext* ctx, JSValueConst v) { return static_cast<std::uint32_t>(number_arg(ctx, v, "joint")); }

JSValue joint_destroy(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  world_of(s).physics().joints().destroy(handle(ctx, argv[0]));
  return JS_UNDEFINED;
}

JSValue joint_alive(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewBool(ctx, world_of(s).physics().joints().alive(handle(ctx, argv[0])));
}

JSValue joint_count(JSContext* ctx, ScriptState& s, JSValueConst*) {
  return JS_NewInt32(ctx, world_of(s).physics().joints().count());
}

// adopt(child, parent, localX, localY)
JSValue adopt(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const core::Vec2 local{static_cast<float>(number_arg(ctx, argv[2], "pos[0]")),
                         static_cast<float>(number_arg(ctx, argv[3], "pos[1]"))};
  world_of(s).adopt(id_arg(ctx, argv[0]), id_arg(ctx, argv[1]), local);
  return JS_UNDEFINED;
}

JSValue parent(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const auto p = world_of(s).parent(id_arg(ctx, argv[0]));
  return p ? JS_NewUint32(ctx, *p) : JS_NULL;
}

JSValue children(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return new_ids(ctx, world_of(s).children(id_arg(ctx, argv[0])));
}

// scale(id[, value]): reads, or sets when given.
JSValue scale(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const ecs::EntityId id = id_arg(ctx, argv[0]);
  if (!JS_IsUndefined(argv[1])) world_of(s).set_scale(id, static_cast<float>(number_arg(ctx, argv[1], "entity.scale")));
  return JS_NewFloat64(ctx, world_of(s).scale(id));
}

// setData(ids, jsonTexts): one call per step mirrors every entity's data.
JSValue set_data(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::vector<double> ids = numbers_arg(ctx, argv[0], "setData ids");
  const std::vector<std::string> texts = strings_arg(ctx, argv[1], "setData texts");
  if (ids.size() != texts.size()) throw core::Error("setData: ids and texts differ in length");
  ecs::World& w = world_of(s);
  for (std::size_t i = 0; i < ids.size(); ++i) w.set_data(static_cast<ecs::EntityId>(ids[i]), texts[i]);
  return JS_UNDEFINED;
}

constexpr std::array<Native, 9> kNatives{{
    {"joint", guarded<joint>, 3},
    {"jointDestroy", guarded<joint_destroy>, 1},
    {"jointAlive", guarded<joint_alive>, 1},
    {"jointCount", guarded<joint_count>, 0},
    {"adopt", guarded<adopt>, 4},
    {"parent", guarded<parent>, 1},
    {"children", guarded<children>, 1},
    {"scale", guarded<scale>, 2},
    {"setData", guarded<set_data>, 2},
}};

}  // namespace

void install_joint_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
