#include "script/system_bindings.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>

#include "core/error.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"

namespace alpha::script {

namespace {

JSValue configure(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.random = core::Random(static_cast<std::uint64_t>(number_arg(ctx, argv[0], "config.seed")));
  s.gravity = {static_cast<float>(number_arg(ctx, argv[1], "config.gravity[0]")),
               static_cast<float>(number_arg(ctx, argv[2], "config.gravity[1]"))};
  return JS_UNDEFINED;
}

JSValue bind_action(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string action = string_arg(ctx, argv[0], "action name");
  const auto inputs = strings_arg(ctx, argv[1], "config.actions." + action);
  s.actions.bind(action, inputs);
  return JS_UNDEFINED;
}

// kind: 0 = pressed, 1 = down, 2 = released
JSValue action(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "action");
  const auto id = s.actions.find(name);
  if (!id) throw core::Error("unknown action \"" + name + "\"; declare it in config.actions");
  const int kind = static_cast<int>(number_arg(ctx, argv[1], "kind"));
  const bool on = kind == 0 ? s.actions.pressed(*id)
                : kind == 1 ? s.actions.down(*id)
                            : s.actions.released(*id);
  return JS_NewBool(ctx, on);
}

// random(min, max, integer): float in [min, max) or integer in [min, max].
JSValue random(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const double min = number_arg(ctx, argv[0], "random min");
  const double max = number_arg(ctx, argv[1], "random max");
  if (max < min) throw core::Error("random(min, max) needs min <= max");
  if (JS_ToBool(ctx, argv[2]) != 1) {
    return JS_NewFloat64(ctx, s.random.range(static_cast<float>(min), static_cast<float>(max)));
  }
  return JS_NewInt32(ctx, s.random.integer(static_cast<std::int32_t>(min), static_cast<std::int32_t>(max)));
}

JSValue timer_every(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewUint32(ctx, s.timers.every(number_arg(ctx, argv[0], "every(seconds)")));
}

JSValue timer_after(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewUint32(ctx, s.timers.after(number_arg(ctx, argv[0], "after(seconds)")));
}

JSValue clear_timers(JSContext*, ScriptState& s, JSValueConst*) {
  s.timers.clear();
  return JS_UNDEFINED;
}

JSValue advance_timers(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return new_ids(ctx, s.timers.advance(number_arg(ctx, argv[0], "dt")));
}

JSValue save_read(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.save_path = s.save_path_for(string_arg(ctx, argv[0], "config.title"));
  std::ifstream file(s.save_path, std::ios::binary);
  std::ostringstream text;
  if (file) text << file.rdbuf();
  return JS_NewString(ctx, text.str().c_str());
}

JSValue save_write(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string json = string_arg(ctx, argv[0], "save data");
  std::ofstream file(s.save_path, std::ios::binary | std::ios::trunc);
  if (!(file << json)) throw core::Error("cannot write save file \"" + s.save_path + "\"");
  return JS_UNDEFINED;
}

// log(level, text): console.* from any instance; stderr plus the dev tools log.
JSValue log(JSContext* ctx, ScriptState&, JSValueConst* argv) {
  const auto level = static_cast<int>(number_arg(ctx, argv[0], "log level"));
  if (level < 0 || level > 2) throw core::Error("log level must be 0, 1 or 2");
  const auto lvl = static_cast<core::LogLevel>(level);
  std::string text = string_arg(ctx, argv[1], "log text");
  core::LogLine line{lvl, std::move(text)};
  std::fputs(core::format_line(line).c_str(), stderr);  // shipping: stderr is the log file
  instances_of(ctx).log().push(line.level, std::move(line.text));
  return JS_UNDEFINED;
}

constexpr std::array<Native, 11> kNatives{{
    {"configure", guarded<configure>, 3},     {"bindAction", guarded<bind_action>, 2},
    {"action", guarded<action>, 2},           {"random", guarded<random>, 3},
    {"timerEvery", guarded<timer_every>, 1},
    {"timerAfter", guarded<timer_after>, 1},  {"clearTimers", guarded<clear_timers>, 0},
    {"advanceTimers", guarded<advance_timers>, 1}, {"saveRead", guarded<save_read>, 1},
    {"saveWrite", guarded<save_write>, 1},    {"log", guarded<log>, 2},
}};

}  // namespace

void install_system_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
