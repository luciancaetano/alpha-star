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

std::string unknown(const ScriptState& s, const std::string& name) {
  std::string have = core::effect_names();
  for (const auto& [shader, ptr] : s.shaders) have += " " + shader;
  return "postfx: unknown effect \"" + name + "\"; have: " + have + " (custom effects: assets.shaders)";
}

// A built-in effect, or an assets.shaders entry with its uniforms.
core::Effect effect(const ScriptState& s, const std::string& name, const core::EffectNumbers& numbers,
                    std::optional<core::Color> color) {
  const auto shader = s.shaders.find(name);
  if (shader == s.shaders.end() && !core::effect_kind(name)) throw core::Error(unknown(s, name));
  if (shader == s.shaders.end()) return core::make_effect(name, numbers, color);
  if (color) throw core::Error("postfx \"" + name + "\": color is not a uniform; shader uniforms are numbers");
  return core::make_shader_effect(shader->second, numbers);
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
  world_of(s).postfx().put(effect(s, name, numbers, color));
  return JS_UNDEFINED;
}

JSValue postfx_remove(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "postfx.remove(name)");
  if (!core::effect_kind(name) && !s.shaders.contains(name)) throw core::Error(unknown(s, name));
  return JS_NewBool(ctx, world_of(s).postfx().remove(name));
}

JSValue postfx_list(JSContext* ctx, ScriptState& s, JSValueConst*) {
  std::vector<std::string> names;
  for (const core::Effect& e : world_of(s).postfx().chain()) names.emplace_back(core::effect_name(e));
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
