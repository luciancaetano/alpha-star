#include "script/system_bindings.hpp"

#include <array>
#include <cstdio>

#include "core/error.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"

namespace alpha::script {

namespace {

JSValue configure(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.random = core::Random(static_cast<std::uint64_t>(number_arg(ctx, argv[0], "config.seed")));
  s.gravity = {static_cast<float>(number_arg(ctx, argv[1], "config.gravity[0]")),
               static_cast<float>(number_arg(ctx, argv[2], "config.gravity[1]"))};
  s.view_size = {static_cast<float>(number_arg(ctx, argv[3], "config.size[0]")),
                 static_cast<float>(number_arg(ctx, argv[4], "config.size[1]"))};
  s.window.size = s.view_size;  // headless runs; the engine writes the real window every frame
  return JS_UNDEFINED;
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

void write_log(JSContext* ctx, core::LogLine line) {
  std::fputs(core::format_line(line).c_str(), stderr);  // shipping: stderr is the log file
  instances_of(ctx).log().push(line.level, std::move(line.text));
}

// log(level, text): console.* from any instance; stderr plus the dev tools log.
JSValue log(JSContext* ctx, ScriptState&, JSValueConst* argv) {
  const auto level = static_cast<int>(number_arg(ctx, argv[0], "log level"));
  if (level < 0 || level > 2) throw core::Error("log level must be 0, 1 or 2");
  write_log(ctx, {static_cast<core::LogLevel>(level), string_arg(ctx, argv[1], "log text")});
  return JS_UNDEFINED;
}

// devWarn(text): a warning for the game's developer (missing translations, ...);
// logged by debug-tools builds only, players never see it.
JSValue dev_warn(JSContext* ctx, ScriptState&, JSValueConst* argv) {
#if ALPHA_DEBUG_TOOLS
  write_log(ctx, {core::LogLevel::Warn, string_arg(ctx, argv[0], "warning")});
#else
  (void)ctx;
  (void)argv;
#endif
  return JS_UNDEFINED;
}

constexpr std::array<Native, 8> kNatives{{
    {"configure", guarded<configure>, 5},     {"random", guarded<random>, 3},
    {"timerEvery", guarded<timer_every>, 1},
    {"timerAfter", guarded<timer_after>, 1},  {"clearTimers", guarded<clear_timers>, 0},
    {"advanceTimers", guarded<advance_timers>, 1}, {"log", guarded<log>, 2},
    {"devWarn", guarded<dev_warn>, 1},
}};

}  // namespace

void install_system_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
