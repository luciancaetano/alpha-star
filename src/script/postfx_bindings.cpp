#include "script/postfx_bindings.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "core/error.hpp"
#include "core/postfx.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/spec_fields.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

namespace {

std::vector<std::string> keys(JSContext* ctx, JSValueConst obj) {
  JSPropertyEnum* tab = nullptr;
  uint32_t len = 0;
  std::vector<std::string> out;
  if (JS_GetOwnPropertyNames(ctx, &tab, &len, obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) return out;
  for (uint32_t i = 0; i < len; ++i) {
    const char* name = JS_AtomToCString(ctx, tab[i].atom);
    if (name) out.emplace_back(name);
    JS_FreeCString(ctx, name);
  }
  JS_FreePropertyEnum(ctx, tab, len);
  return out;
}

// postfxPut(name, params?): adds the effect or sets its params (order kept).
JSValue postfx_put(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "postfx.add(name)");
  core::EffectNumbers numbers;
  std::optional<core::Color> color;
  if (!JS_IsUndefined(argv[1]) && !JS_IsObject(argv[1])) {
    throw core::Error("postfx.add(\"" + name + "\", params): params must be an object, got " + describe(ctx, argv[1]));
  }
  for (const std::string& key : JS_IsObject(argv[1]) ? keys(ctx, argv[1]) : std::vector<std::string>{}) {
    const Value v = field(ctx, argv[1], key.c_str());
    const std::string what = "postfx \"" + name + "\"." + key;
    if (key == "color") color = color_named(ctx, v.get(), what);
    else numbers[key] = static_cast<float>(number_arg(ctx, v.get(), what));
  }
  world_of(s).postfx().put(core::make_effect(name, numbers, color));
  return JS_UNDEFINED;
}

core::EffectKind kind_arg(JSContext* ctx, JSValueConst v) {
  const std::string name = string_arg(ctx, v, "postfx.remove(name)");
  const auto kind = core::effect_kind(name);
  if (!kind) throw core::Error("postfx: unknown effect \"" + name + "\"; have: " + core::effect_names());
  return *kind;
}

JSValue postfx_remove(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewBool(ctx, world_of(s).postfx().remove(kind_arg(ctx, argv[0])));
}

JSValue postfx_list(JSContext* ctx, ScriptState& s, JSValueConst*) {
  std::vector<std::string> names;
  for (const core::Effect& e : world_of(s).postfx().chain()) names.emplace_back(core::effect_name(e.kind));
  return new_strings(ctx, names);
}

JSValue postfx_clear(JSContext*, ScriptState& s, JSValueConst*) {
  world_of(s).postfx().clear();
  return JS_UNDEFINED;
}

// setLighting(ambient | null): config.lighting.ambient, or lighting off.
JSValue set_lighting(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  if (JS_IsNull(argv[0])) s.lighting.reset();
  else s.lighting = core::Lighting{color_named(ctx, argv[0], "config.lighting.ambient")};
  return JS_UNDEFINED;
}

constexpr std::array<Native, 5> kNatives{{
    {"postfxPut", guarded<postfx_put>, 2},
    {"postfxRemove", guarded<postfx_remove>, 1},
    {"postfxList", guarded<postfx_list>, 0},
    {"postfxClear", guarded<postfx_clear>, 0},
    {"setLighting", guarded<set_lighting>, 1},
}};

}  // namespace

void install_postfx_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

core::LightSpec parse_light(JSContext* ctx, JSValueConst obj) {
  if (!JS_IsObject(obj)) throw core::Error(kSpawn + "light must be { radius, color?, intensity? }, got " + describe(ctx, obj));
  if (!has(ctx, obj, "radius")) throw core::Error(kSpawn + "light.radius is required (world pixels)");
  const float radius = number_or(ctx, obj, "radius", "light.radius", 0.0f);
  const core::Color color = has(ctx, obj, "color") ? color_arg(ctx, field(ctx, obj, "color").get(), "light.color")
                                                   : core::Color{255, 255, 255, 255};
  return core::make_light(radius, color, number_or(ctx, obj, "intensity", "light.intensity", 1.0f));
}

}  // namespace alpha::script
