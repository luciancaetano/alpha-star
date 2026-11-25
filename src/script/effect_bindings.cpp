#include "script/effect_bindings.hpp"

#include <array>
#include <cstdio>
#include <string>

#include "core/error.hpp"
#include "script/effect_parser.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/spec_fields.hpp"
#include "script/text_look.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

namespace {

std::string hex(core::Color c) {
  std::array<char, 10> text{};
  std::snprintf(text.data(), text.size(), "#%02x%02x%02x%02x", c.r, c.g, c.b, c.a);
  return text.data();
}

const char* blend_name(core::Blend b) {
  if (b == core::Blend::Add) return "add";
  if (b == core::Blend::Multiply) return "multiply";
  return "alpha";
}

JSValue new_string(JSContext* ctx, const std::string& s) { return JS_NewStringLen(ctx, s.data(), s.size()); }

// look(id) -> [tint, alpha, blend]
JSValue look(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const core::ShapeSpec& shape = world_of(s).look(id_arg(ctx, argv[0]));
  JSValue out = JS_NewArray(ctx);
  JS_SetPropertyUint32(ctx, out, 0, new_string(ctx, hex(shape.tint)));
  JS_SetPropertyUint32(ctx, out, 1, JS_NewFloat64(ctx, shape.alpha));
  JS_SetPropertyUint32(ctx, out, 2, JS_NewString(ctx, blend_name(shape.blend)));
  return out;
}

// setLook(id, tint, alpha, blend)
JSValue set_look(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  core::ShapeSpec& shape = world_of(s).look(id_arg(ctx, argv[0]));
  const core::Color tint = color_arg(ctx, argv[1], "tint");
  const double alpha = number_arg(ctx, argv[2], "alpha");
  if (!(alpha >= 0.0 && alpha <= 1.0)) throw core::Error("entity alpha must be 0..1, got " + std::to_string(alpha));
  shape.blend = parse_blend(ctx, argv[3], "blend");
  shape.tint = tint;
  shape.alpha = static_cast<float>(alpha);
  return JS_UNDEFINED;
}

core::ShapeSpec& text_shape(JSContext* ctx, ScriptState& s, JSValueConst id) {
  core::ShapeSpec& shape = world_of(s).look(id_arg(ctx, id));
  if (!shape.text) throw core::Error("entity " + std::to_string(id_arg(ctx, id)) + " is not text; spawn it with shape { text }");
  return shape;
}

// text(id) -> string; text(id, value) relays the line out.
JSValue text(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  core::ShapeSpec& shape = text_shape(ctx, s, argv[0]);
  if (JS_IsUndefined(argv[1])) return new_string(ctx, shape.text->text);
  shape.text->text = to_string(ctx, argv[1]);
  lay_out(s, shape);
  return JS_UNDEFINED;
}

JSValue layer(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return new_string(ctx, world_of(s).layer(id_arg(ctx, argv[0])).name);
}

// defineLayer(name, z, parallax): config.layers entry.
JSValue define_layer(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "config.layers name");
  const double z = number_arg(ctx, argv[1], "config.layers." + name + ".z");
  const double parallax = number_arg(ctx, argv[2], "config.layers." + name + ".parallax");
  if (parallax < 0.0 || parallax > 10.0) throw core::Error("config.layers." + name + ".parallax must be 0..10 (1 = moves with the world)");
  s.layers[name] = {name, static_cast<int>(z), static_cast<float>(parallax)};
  return JS_UNDEFINED;
}

// emit(id, n): a burst of n on the next step.
JSValue emit(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const double n = number_arg(ctx, argv[1], "emit(count)");
  if (!(n >= 0.0 && n <= 20000.0)) throw core::Error("emit(count): count must be 0..20000");
  world_of(s).emitter(id_arg(ctx, argv[0])).burst(static_cast<int>(n));
  return JS_UNDEFINED;
}

// emitting(id) -> bool; emitting(id, on) switches `rate` emission.
JSValue emitting(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  core::Emitter& e = world_of(s).emitter(id_arg(ctx, argv[0]));
  if (JS_IsUndefined(argv[1])) return JS_NewBool(ctx, e.emitting());
  e.set_emitting(JS_ToBool(ctx, argv[1]) == 1);
  return JS_UNDEFINED;
}

// particles(id) -> live particles of the entity; particles() -> of the world.
JSValue particles(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  ecs::World& w = world_of(s);
  if (JS_IsUndefined(argv[0])) return JS_NewInt32(ctx, w.particle_count());
  const ecs::EntityId id = id_arg(ctx, argv[0]);
  return JS_NewInt32(ctx, w.has_emitter(id) ? w.emitter(id).count() : 0);
}

// windowGet() -> [fullscreen, width, height]
JSValue window_get(JSContext* ctx, ScriptState& s, JSValueConst*) {
  JSValue out = JS_NewArray(ctx);
  JS_SetPropertyUint32(ctx, out, 0, JS_NewBool(ctx, s.window.request.value_or(s.window.fullscreen)));
  JS_SetPropertyUint32(ctx, out, 1, JS_NewFloat64(ctx, s.window.size.x));
  JS_SetPropertyUint32(ctx, out, 2, JS_NewFloat64(ctx, s.window.size.y));
  return out;
}

JSValue window_fullscreen(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.window.request = JS_ToBool(ctx, argv[0]) == 1;
  return JS_UNDEFINED;
}

constexpr std::array<Native, 10> kNatives{{
    {"look", guarded<look>, 1},
    {"setLook", guarded<set_look>, 4},
    {"text", guarded<text>, 2},
    {"layer", guarded<layer>, 1},
    {"defineLayer", guarded<define_layer>, 3},
    {"emit", guarded<emit>, 2},
    {"emitting", guarded<emitting>, 2},
    {"particles", guarded<particles>, 1},
    {"windowGet", guarded<window_get>, 0},
    {"windowFullscreen", guarded<window_fullscreen>, 1},
}};

}  // namespace

void install_effect_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
