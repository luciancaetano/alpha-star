#include "script/input_bindings.hpp"

#include <array>
#include <string>

#include "core/error.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"

namespace alpha::script {

namespace {

// bind(table, name, list): table "actions" (list = inputs) or "axes" (list = sources).
JSValue bind(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string table = string_arg(ctx, argv[0], "config table");
  const std::string name = string_arg(ctx, argv[1], "config." + table + " name");
  const auto list = strings_arg(ctx, argv[2], "config." + table + "." + name);
  if (table == "axes") {
    s.controls.axes.bind(name, list);
  } else {
    s.controls.actions.bind(name, list);
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

constexpr std::array<Native, 7> kNatives{{
    {"mouse", guarded<mouse>, 0},
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

void install_input_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
