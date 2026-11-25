#include "script/camera_bindings.hpp"

#include <array>
#include <cmath>
#include <string>

#include "core/error.hpp"
#include "core/particles.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/spec_fields.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

namespace {

core::Camera& camera(ScriptState& s) { return world_of(s).camera(); }

// argv[i] as a number in [lo, hi] (`what` names it in the error).
JSValue camera_get(JSContext* ctx, ScriptState& s, JSValueConst*) {
  const core::Camera& c = camera(s);
  JSValue out = JS_NewArray(ctx);
  const std::array<double, 4> values{c.pos.x, c.pos.y, c.zoom, c.rotation};
  for (std::uint32_t i = 0; i < values.size(); ++i) JS_SetPropertyUint32(ctx, out, i, JS_NewFloat64(ctx, values[i]));
  return out;
}

// cameraSet(x, y, zoom, rotation)
JSValue camera_set(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  core::Camera& c = camera(s);
  c.pos = {ranged(ctx, argv[0], "camera.pos.x", -1e9, 1e9), ranged(ctx, argv[1], "camera.pos.y", -1e9, 1e9)};
  c.zoom = ranged(ctx, argv[2], "camera.zoom", 0.01, 100.0);
  c.rotation = ranged(ctx, argv[3], "camera.rotation", -1e6, 1e6);
  return JS_UNDEFINED;
}

// cameraFollow(id | null, lerp, deadzoneW, deadzoneH)
JSValue camera_follow(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  ecs::World& w = world_of(s);
  if (JS_IsNull(argv[0])) {
    w.follow(std::nullopt);
    return JS_UNDEFINED;
  }
  const ecs::EntityId id = id_arg(ctx, argv[0]);
  if (!w.alive(id)) throw core::Error("camera.follow: entity " + std::to_string(id) + " is not alive");
  w.camera().follow = core::Follow{ranged(ctx, argv[1], "camera.follow lerp", 0.001, 1.0),
                                   {ranged(ctx, argv[2], "camera.follow deadzone[0]", 0.0, 1e6),
                                    ranged(ctx, argv[3], "camera.follow deadzone[1]", 0.0, 1e6)}};
  w.follow(id);
  return JS_UNDEFINED;
}

// cameraShake(intensity, seconds); 0 seconds stops a shake.
JSValue camera_shake(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const float intensity = ranged(ctx, argv[0], "camera.shake intensity", 0.0, 10000.0);
  core::start_shake(camera(s), s.reduced_motion ? intensity * core::kReducedMotion : intensity,
                    ranged(ctx, argv[1], "camera.shake seconds", 0.0, 60.0));
  return JS_UNDEFINED;
}

// cameraBounds(x, y, w, h) or cameraBounds(null) to remove them.
JSValue camera_bounds(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  core::Camera& c = camera(s);
  if (JS_IsNull(argv[0])) {
    c.bounds.reset();
    return JS_UNDEFINED;
  }
  const core::Vec2 at{ranged(ctx, argv[0], "camera.bounds x", -1e9, 1e9), ranged(ctx, argv[1], "camera.bounds y", -1e9, 1e9)};
  const core::Vec2 size{ranged(ctx, argv[2], "camera.bounds width", 1.0, 1e9),
                        ranged(ctx, argv[3], "camera.bounds height", 1.0, 1e9)};
  c.bounds = core::Box{at, at + size};
  return JS_UNDEFINED;
}

// toWorld(sx, sy): game-pixel screen point -> world point, through the current camera.
JSValue to_world(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const core::Vec2 screen{static_cast<float>(number_arg(ctx, argv[0], "camera.toWorld x")),
                          static_cast<float>(number_arg(ctx, argv[1], "camera.toWorld y"))};
  return new_vec2(ctx, core::to_world(core::view_of(camera(s)), screen));
}

constexpr std::array<Native, 6> kNatives{{
    {"cameraGet", guarded<camera_get>, 0},       {"cameraSet", guarded<camera_set>, 4},
    {"cameraFollow", guarded<camera_follow>, 4}, {"cameraShake", guarded<camera_shake>, 2},
    {"cameraBounds", guarded<camera_bounds>, 4}, {"toWorld", guarded<to_world>, 2},
}};

}  // namespace

void install_camera_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
