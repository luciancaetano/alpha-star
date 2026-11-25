#include "script/ui_bindings.hpp"

#include <array>
#include <memory>
#include <utility>

#include "core/error.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/sprite_bindings.hpp"

namespace alpha::script {

namespace {

ui::Ui& ui_of(ScriptState& s) {
  if (!s.ui) throw core::Error("the game UI exists only after boot");
  return *s.ui;
}

std::string arg(JSContext* ctx, JSValueConst* argv, int i, const char* what) {
  return string_arg(ctx, argv[i], what);
}

// uiCreate(width, height): the instance's UI context, sized to config.size.
JSValue ui_create(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.ui.reset();
  s.ui = std::make_unique<ui::Ui>(static_cast<int>(number_arg(ctx, argv[0], "config.size[0]")),
                                  static_cast<int>(number_arg(ctx, argv[1], "config.size[1]")));
  return JS_UNDEFINED;
}

// argv = name, file of assets.<kind>.<name>; returns {name, full path}.
std::pair<std::string, std::string> asset(JSContext* ctx, ScriptState& s, JSValueConst* argv, const std::string& kind) {
  const std::string name = arg(ctx, argv, 0, "asset name");
  return {name, (s.asset_dir / arg(ctx, argv, 1, ("assets." + kind + "." + name).c_str())).string()};
}

JSValue load_ui(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const auto [name, path] = asset(ctx, s, argv, "ui");
  ui_of(s).add(name, path);
  return JS_UNDEFINED;
}

JSValue load_font(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  ui_of(s);  // fonts need the UI library alive
  const auto [name, path] = asset(ctx, s, argv, "fonts");
  ui::load_font(path, name);
  s.fonts[name] = path;  // world text rasterizes it too
  if (smooth_arg(ctx, argv[2])) s.smooth_fonts.insert(name);
  else s.smooth_fonts.erase(name);
  return JS_UNDEFINED;
}

// argv[0], argv[1] = document name, element id.
std::pair<std::string, std::string> target(JSContext* ctx, JSValueConst* argv) {
  return {arg(ctx, argv, 0, "document name"), arg(ctx, argv, 1, "element id")};
}

JSValue ui_show(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  ui_of(s).show(arg(ctx, argv, 0, "document name"));
  return JS_UNDEFINED;
}

JSValue ui_hide(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  ui_of(s).hide(arg(ctx, argv, 0, "document name"));
  return JS_UNDEFINED;
}

JSValue ui_set(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const auto [doc, id] = target(ctx, argv);
  ui_of(s).set(doc, id, to_string(ctx, argv[2]));
  return JS_UNDEFINED;
}

JSValue ui_on(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string event = arg(ctx, argv, 2, "event");
  const auto [doc, id] = target(ctx, argv);
  const std::uint32_t handler = ui_of(s).on(doc, id, event);
  return JS_NewUint32(ctx, handler);
}

JSValue ui_events(JSContext* ctx, ScriptState& s, JSValueConst*) {
  const auto events = ui_of(s).take_events();
  return new_ids(ctx, events);
}

JSValue ui_reset(JSContext*, ScriptState& s, JSValueConst*) {
  ui_of(s).reset();
  return JS_UNDEFINED;
}

#if ALPHA_DEBUG_TOOLS  // simulate() natives, up to kTestNatives
JSValue ui_click(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const auto [doc, id] = target(ctx, argv);
  ui_of(s).click(doc, id);
  return JS_UNDEFINED;
}

// uiView(): { doc: { shown, texts: { id: text } } } for alpha/test expectations.
JSValue ui_view(JSContext* ctx, ScriptState& s, JSValueConst*) {
  JSValue out = JS_NewObject(ctx);
  for (const auto& [name, view] : ui_of(s).view()) {
    JSValue doc = JS_NewObject(ctx);
    JSValue texts = JS_NewObject(ctx);
    for (const auto& [id, text] : view.texts) JS_SetPropertyStr(ctx, texts, id.c_str(), JS_NewString(ctx, text.c_str()));
    JS_SetPropertyStr(ctx, doc, "shown", JS_NewBool(ctx, view.shown));
    JS_SetPropertyStr(ctx, doc, "texts", texts);
    JS_SetPropertyStr(ctx, out, name.c_str(), doc);
  }
  return out;
}

constexpr std::array<Native, 2> kTestNatives{{
    {"uiClick", guarded<ui_click>, 2}, {"uiView", guarded<ui_view>, 0}}};
#endif

constexpr std::array<Native, 9> kNatives{{
    {"uiCreate", guarded<ui_create>, 2}, {"loadUi", guarded<load_ui>, 2},
    {"loadFont", guarded<load_font>, 3}, {"uiShow", guarded<ui_show>, 1},
    {"uiHide", guarded<ui_hide>, 1},     {"uiSet", guarded<ui_set>, 3},
    {"uiOn", guarded<ui_on>, 3},         {"uiEvents", guarded<ui_events>, 0},
    {"uiReset", guarded<ui_reset>, 0},
}};

}  // namespace

void install_ui_bindings(JSContext* ctx, JSValueConst target) {
  install(ctx, target, kNatives);
#if ALPHA_DEBUG_TOOLS
  install(ctx, target, kTestNatives);
#endif
}

}  // namespace alpha::script
