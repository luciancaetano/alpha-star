#include "script/instance_bindings.hpp"

#include <array>
#include <string>

#include "core/error.hpp"
#include "script/js_engine.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"

namespace alpha::script {

namespace {

std::uint32_t instance_arg(JSContext* ctx, JSValueConst v) {
  return static_cast<std::uint32_t>(number_arg(ctx, v, "instance id"));
}

JSValue instance_use(JSContext* ctx, ScriptState&, JSValueConst* argv) {
  return JS_NewUint32(ctx, instances_of(ctx).use(instance_arg(ctx, argv[0])));
}

#if ALPHA_DEBUG_TOOLS  // simulate() natives, up to kTestNatives
JSValue instance_create(JSContext* ctx, ScriptState&, JSValueConst*) {
  return JS_NewUint32(ctx, instances_of(ctx).create());
}

JSValue instance_destroy(JSContext* ctx, ScriptState&, JSValueConst* argv) {
  instances_of(ctx).destroy(instance_arg(ctx, argv[0]));
  return JS_UNDEFINED;
}

// holdActions(names): one step with exactly these actions held, as if their inputs were.
JSValue hold_actions(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  core::InputSet held;
  for (const std::string& name : strings_arg(ctx, argv[0], "held actions")) {
    const auto id = s.actions.find(name);
    if (!id) throw core::Error("unknown action \"" + name + "\"; declare it in config.actions");
    held |= s.actions.inputs(*id);
  }
  s.actions.update(held);
  return JS_UNDEFINED;
}

// tagCounts(): { tag: number of living entities with it } for every tag ever used.
JSValue tag_counts(JSContext* ctx, ScriptState& s, JSValueConst*) {
  if (!s.world) throw core::Error("no active scene world");
  JSValue out = JS_NewObject(ctx);
  for (std::size_t i = 0; i < s.tags.size(); ++i) {
    const auto id = static_cast<core::TagId>(i);
    const auto count = static_cast<std::uint32_t>(s.world->find_all(id).size());
    JS_SetPropertyStr(ctx, out, std::string(s.tags.name(id)).c_str(), JS_NewUint32(ctx, count));
  }
  return out;
}

JSValue step_budget(JSContext* ctx, ScriptState&, JSValueConst*) {
  Engine::of(ctx).restart_budget();
  return JS_UNDEFINED;
}

// simulate() only: absent from shipping builds.
constexpr std::array<Native, 5> kTestNatives{{
    {"instanceCreate", guarded<instance_create>, 0}, {"instanceDestroy", guarded<instance_destroy>, 1},
    {"holdActions", guarded<hold_actions>, 1},       {"tagCounts", guarded<tag_counts>, 0},
    {"stepBudget", guarded<step_budget>, 0},
}};
#endif

constexpr std::array<Native, 1> kNatives{{{"instanceUse", guarded<instance_use>, 1}}};

}  // namespace

void install_instance_bindings(JSContext* ctx, JSValueConst target) {
  install(ctx, target, kNatives);
#if ALPHA_DEBUG_TOOLS
  install(ctx, target, kTestNatives);
#endif
}

}  // namespace alpha::script
