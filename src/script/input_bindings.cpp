#include "script/input_bindings.hpp"

#include <array>
#include <string>

#include "core/error.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/touch_config.hpp"

namespace alpha::script {

namespace {

// bind(table, name, list): table "actions" (list = inputs) or "axes" (list = sources).
JSValue bind(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string table = string_arg(ctx, argv[0], "config table");
  const std::string name = string_arg(ctx, argv[1], "config." + table + " name");
  const auto list = strings_arg(ctx, argv[2], "config." + table + "." + name);
  if (table == "axes") {
    s.controls.axes.bind(name, list, &s.controls.touch);
  } else {
    s.controls.actions.bind(name, list, &s.controls.touch);
  }
  return JS_UNDEFINED;
}

JSValue deadzone(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.controls.axes.set_deadzone(static_cast<float>(number_arg(ctx, argv[0], "config.deadzone")));
  return JS_UNDEFINED;
}

// action(name, kind, player): kind 0 = pressed, 1 = down, 2 = released.
JSValue action(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "action");
  const auto id = s.controls.actions.find(name);
  if (!id) throw core::Error("unknown action \"" + name + "\"; declare it in config.actions");
  const int kind = static_cast<int>(number_arg(ctx, argv[1], "kind"));
  const std::size_t p = player_arg(ctx, argv[2]);
  const auto& a = s.controls.actions;
  const bool on = kind == 0 ? a.pressed(*id, p) : kind == 1 ? a.down(*id, p) : a.released(*id, p);
  return JS_NewBool(ctx, on);
}

JSValue axis(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "axis");
  const auto id = s.controls.axes.find(name);
  if (!id) {
    const std::string declared = s.controls.axes.names();
    throw core::Error("unknown axis \"" + name + "\"; config.axes declares: " +
                      (declared.empty() ? "(none)" : declared));
  }
  return JS_NewFloat64(ctx, s.controls.axes.value(*id, player_arg(ctx, argv[1])));
}

JSValue players(JSContext* ctx, ScriptState& s, JSValueConst*) {
  return JS_NewUint32(ctx, static_cast<std::uint32_t>(s.controls.players));
}

JSValue rumble(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const auto strength = static_cast<float>(number_arg(ctx, argv[0], "rumble strength"));
  const auto seconds = static_cast<float>(number_arg(ctx, argv[1], "rumble seconds"));
  s.controls.request_rumble(strength, seconds, player_arg(ctx, argv[2]));
  return JS_UNDEFINED;
}

JSValue mouse(JSContext* ctx, ScriptState& s, JSValueConst*) { return new_vec2(ctx, s.controls.mouse); }

// touchControls(config.touchSticks, config.touchButtons, mobile): before any
// action or axis binds (they may name the controls' sources).
JSValue touch_controls(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  auto sticks = touch_sticks(ctx, s, argv[0]);
  auto buttons = touch_buttons(ctx, s, argv[1]);
  s.controls.touch.configure(std::move(sticks), std::move(buttons), JS_ToBool(ctx, argv[2]) == 1);
  s.controls.touch.set_screen(touch_screen(s));
  return JS_UNDEFINED;
}

// touchScene(name): a scene started (touch controls of other scenes go inert).
JSValue touch_scene(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.controls.touch.enter_scene(string_arg(ctx, argv[0], "scene"));
  return JS_UNDEFINED;
}

JSValue touch_release(JSContext*, ScriptState& s, JSValueConst*) {
  s.controls.touch.release_all();
  return JS_UNDEFINED;
}

// touches() -> [{ id, pos, pressed, released }]: this step's fingers no touch control or UI took.
JSValue touches(JSContext* ctx, ScriptState& s, JSValueConst*) {
  JSValue out = JS_NewArray(ctx);
  std::uint32_t i = 0;
  for (const core::Finger& f : s.controls.fingers) {
    JSValue t = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, t, "id", JS_NewFloat64(ctx, static_cast<double>(f.id)));
    JS_SetPropertyStr(ctx, t, "pos", new_vec2(ctx, f.pos));
    JS_SetPropertyStr(ctx, t, "pressed", JS_NewBool(ctx, f.began));
    JS_SetPropertyStr(ctx, t, "released", JS_NewBool(ctx, f.ended));
    JS_SetPropertyUint32(ctx, out, i++, t);
  }
  return out;
}

constexpr std::array<Native, 11> kNatives{{
    {"touchScene", guarded<touch_scene>, 1},
    {"touchRelease", guarded<touch_release>, 0},
    {"mouse", guarded<mouse>, 0},            {"touchControls", guarded<touch_controls>, 3},
    {"touches", guarded<touches>, 0},
    {"bind", guarded<bind>, 3},
    {"deadzone", guarded<deadzone>, 1},      {"action", guarded<action>, 3},
    {"axis", guarded<axis>, 2},              {"players", guarded<players>, 0},
    {"rumble", guarded<rumble>, 3},
}};

}  // namespace

std::size_t player_arg(JSContext* ctx, JSValueConst v) {
  const double n = number_arg(ctx, v, "player");
  if (!(n >= 0 && n <= static_cast<double>(core::kMaxPlayers)) || n != static_cast<double>(static_cast<int>(n))) {
    throw core::Error("player must be an integer 1.." + std::to_string(core::kMaxPlayers) + ", got " +
                      to_string(ctx, v));
  }
  return n == 0 ? core::kAnyPlayer : static_cast<std::size_t>(n) - 1;
}

core::TouchScreen touch_screen(const ScriptState& s) {
  const core::Vec2 view = s.view_size.x > 0.0f ? s.view_size : s.design_size;
  const core::Insets& in = s.window.safe;
  return {{{in.left, in.top}, {view.x - in.right, view.y - in.bottom}}, view};
}

void install_input_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
