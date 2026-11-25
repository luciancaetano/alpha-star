#include "script/settings_bindings.hpp"

#include <array>
#include <map>

#include "core/error.hpp"
#include "core/os.hpp"
#include "platform/mobile.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/spec_fields.hpp"

namespace alpha::script {

namespace {

ui::Ui& game_ui(ScriptState& s) {
  if (!s.ui) throw core::Error("the game UI exists only after boot");
  return *s.ui;
}

// uiKeys(): every data-t key of the game's documents.
JSValue ui_keys(JSContext* ctx, ScriptState& s, JSValueConst*) {
  const auto keys = game_ui(s).translation_keys();
  return new_strings(ctx, keys);
}

// uiTranslate(keys, texts): the text each data-t key shows.
JSValue ui_translate(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const auto keys = strings_arg(ctx, argv[0], "translation keys");
  const auto texts = strings_arg(ctx, argv[1], "translations");
  if (keys.size() != texts.size()) throw core::Error("uiTranslate: one text per key");
  std::map<std::string, std::string> out;
  for (std::size_t i = 0; i < keys.size(); ++i) out[keys[i]] = texts[i];
  game_ui(s).translate(std::move(out));
  return JS_UNDEFINED;
}

// uiAccess(scale, highContrast)
JSValue ui_access(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const float scale = ranged(ctx, argv[0], "settings.uiScale", 0.5, 3.0);
  game_ui(s).accessibility(scale, JS_ToBool(ctx, argv[1]) == 1);
  return JS_UNDEFINED;
}

JSValue reduced_motion(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.reduced_motion = JS_ToBool(ctx, argv[0]) == 1;
  return JS_UNDEFINED;
}

JSValue locale_set(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.locale = string_arg(ctx, argv[0], "locale");
  return JS_UNDEFINED;
}

// platformOs(): ctx.platform.os ("windows", "android", ...).
JSValue platform_os(JSContext* ctx, ScriptState&, JSValueConst*) {
  const std::string_view name = core::os_name(platform::running_os());
  return JS_NewStringLen(ctx, name.data(), name.size());
}

constexpr std::array<Native, 6> kNatives{{
    {"uiKeys", guarded<ui_keys>, 0},
    {"uiTranslate", guarded<ui_translate>, 2},
    {"uiAccess", guarded<ui_access>, 2},
    {"reducedMotion", guarded<reduced_motion>, 1},
    {"localeSet", guarded<locale_set>, 1},
    {"platformOs", guarded<platform_os>, 0},
}};

}  // namespace

void install_settings_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
