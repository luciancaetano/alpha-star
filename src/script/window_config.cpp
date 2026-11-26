#include "script/window_config.hpp"

#include <string>

#include "core/error.hpp"

namespace alpha::script {

namespace {

// What replaces each old config.window.scaling value.
std::string scaling_replacement(const std::string& old) {
  if (old == "letterbox") return "{ aspect: \"keep\" }";
  if (old == "integer") return "{ scale: \"integer\" } (add aspect: \"keep\" for bars instead of more world)";
  if (old == "expand") return "nothing: aspect \"expand\" is the default";
  return "{ aspect: \"expand\" } (\"stretch\" and \"crop\" were removed; place things inside ctx.camera.view)";
}

[[noreturn]] void scaling_removed(JSContext* ctx, const Value& scaling) {
  const std::string old = to_string(ctx, scaling.get());
  throw core::Error("config.window.scaling was replaced by config.window.aspect (\"expand\" default, \"keep\", "
                    "\"keep_width\", \"keep_height\") and config.window.scale (\"fractional\" default, \"integer\"); "
                    "for scaling \"" + old + "\" use " + scaling_replacement(old));
}

// config.window[name] parsed by `parse`, or `fallback` when absent; `choices` names the valid values.
template <typename T, typename Parse>
T named(JSContext* ctx, const Value& w, const char* name, T fallback, Parse parse, const char* choices) {
  const Value v = w.prop(name);
  if (JS_IsUndefined(v.get())) return fallback;
  const std::string what = std::string("config.window.") + name;
  const std::string text = string_arg(ctx, v.get(), what);
  const auto parsed = parse(text);
  if (!parsed) throw core::Error(what + " must be " + choices + ", got \"" + text + "\"");
  return *parsed;
}

}  // namespace

bool flag_or(JSContext* ctx, const Value& obj, const char* name, bool fallback) {
  const Value v = obj.prop(name);
  return JS_IsUndefined(v.get()) ? fallback : JS_ToBool(ctx, v.get()) == 1;
}

core::WindowConfig window_config(JSContext* ctx, const Value& info) {
  const Value w = info.prop("window");
  core::WindowConfig c;
  if (JS_IsUndefined(w.get())) return c;
  if (!JS_IsObject(w.get())) {
    throw core::Error("config.window must be { fullscreen?, aspect?, scale?, resizable?, altEnter?, borderless?, alwaysOnTop?, maximized?, vsync? }");
  }
  if (const Value old = w.prop("scaling"); !JS_IsUndefined(old.get())) scaling_removed(ctx, old);
  c.fullscreen = flag_or(ctx, w, "fullscreen", false);
  c.resizable = flag_or(ctx, w, "resizable", true);
  c.alt_enter = flag_or(ctx, w, "altEnter", true);
  c.borderless = flag_or(ctx, w, "borderless", false);
  c.always_on_top = flag_or(ctx, w, "alwaysOnTop", false);
  c.maximized = flag_or(ctx, w, "maximized", false);
  c.vsync = flag_or(ctx, w, "vsync", true);
  c.stretch = {named(ctx, w, "aspect", core::Aspect::Expand, core::parse_aspect, "\"expand\", \"keep\", \"keep_width\" or \"keep_height\""),
               named(ctx, w, "scale", core::Scale::Fractional, core::parse_scale, "\"fractional\" or \"integer\"")};
  return c;
}

}  // namespace alpha::script
