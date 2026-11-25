#include "script/effect_parser.hpp"

#include <cmath>
#include <utility>

#include "core/particles.hpp"
#include "script/spec_fields.hpp"
#include "script/text_look.hpp"

namespace alpha::script {

namespace {

const std::string kEmitter = "emitter.";

core::Align parse_align(JSContext* ctx, JSValueConst v) {
  constexpr std::array<std::pair<const char*, core::Align>, 3> kAligns{
      {{"left", core::Align::Left}, {"center", core::Align::Center}, {"right", core::Align::Right}}};
  return choice(ctx, v, "shape.align", kAligns);
}

// A number (both ends) or [a, b].
core::Span span(JSContext* ctx, JSValueConst obj, const char* name, core::Span fallback) {
  if (!has(ctx, obj, name)) return fallback;
  const Value v = field(ctx, obj, name);
  const std::string path = kSpawn + kEmitter + name;
  if (JS_IsNumber(v.get())) {
    const auto n = static_cast<float>(number_arg(ctx, v.get(), path));
    return {n, n};
  }
  const core::Vec2 ab = vec2_arg(ctx, v.get(), path + " (a number or [min, max])");
  return {ab.x, ab.y};
}

// "#fff" or ["#fff", "#fff0"] (start, end).
void colors(JSContext* ctx, JSValueConst obj, core::EmitterSpec& out) {
  if (!has(ctx, obj, "color")) return;
  const Value v = field(ctx, obj, "color");
  if (JS_IsString(v.get())) {
    out.color_start = out.color_end = color_arg(ctx, v.get(), kEmitter + "color");
    return;
  }
  const Value a(ctx, JS_GetPropertyUint32(ctx, v.get(), 0));
  const Value b(ctx, JS_GetPropertyUint32(ctx, v.get(), 1));
  out.color_start = color_arg(ctx, a.get(), kEmitter + "color[0]");
  out.color_end = color_arg(ctx, b.get(), kEmitter + "color[1]");
}

core::ShapeSpec picture_look(const core::ImagePtr& image, core::Region src) {
  core::ShapeSpec look{core::Rect{{1, 1}}, {255, 255, 255, 255}, {}, image};
  look.src = src;
  return look;
}

core::ShapeSpec sheet_look(JSContext* ctx, JSValueConst v, const ScriptState& s) {
  const std::string name = string_arg(ctx, field(ctx, v, "sheet").get(), kSpawn + "emitter.shape.sheet");
  const core::SheetPtr sheet = named(s.sheets, name, "emitter.shape.sheet", "assets.sheets");
  const auto frame = static_cast<int>(number_or(ctx, v, "frame", kEmitter + "shape.frame", 0));
  if (frame < 0 || frame >= sheet->frame_count()) {
    throw core::Error(kSpawn + "emitter.shape.frame must be 0.." + std::to_string(sheet->frame_count() - 1) +
                      " (frames of sheet \"" + name + "\"), got " + std::to_string(frame));
  }
  return picture_look(sheet->picture.image, sheet->frame(frame));
}

// "circle" | "rect" | { image } | { sheet, frame? }
core::ShapeSpec particle_look(JSContext* ctx, JSValueConst obj, const ScriptState& s) {
  if (!has(ctx, obj, "shape")) return core::EmitterSpec{}.look;
  const Value v = field(ctx, obj, "shape");
  if (JS_IsString(v.get())) {
    const std::string name = string_arg(ctx, v.get(), kSpawn + "emitter.shape");
    if (name == "circle") return core::EmitterSpec{}.look;
    if (name == "rect") return {core::Rect{{1, 1}}, {255, 255, 255, 255}};
    throw core::Error(kSpawn + "emitter.shape must be \"circle\", \"rect\", { image } or { sheet, frame }, got \"" + name + "\"");
  }
  if (has(ctx, v.get(), "sheet")) return sheet_look(ctx, v.get(), s);
  const std::string name = string_arg(ctx, field(ctx, v.get(), "image").get(), kSpawn + "emitter.shape.image");
  const core::Picture pic = named(s.images, name, "emitter.shape.image", "assets.images");
  return picture_look(pic.image, pic.region);
}

void check(bool ok, const std::string& what) {
  if (!ok) throw core::Error(kSpawn + kEmitter + what);
}

void counts(JSContext* ctx, JSValueConst obj, core::EmitterSpec& out) {
  out.rate = number_or(ctx, obj, "rate", kEmitter + "rate", 0);
  out.burst = static_cast<int>(number_or(ctx, obj, "burst", kEmitter + "burst", 0));
  out.max = static_cast<int>(number_or(ctx, obj, "max", kEmitter + "max", 100));
  out.one_shot = JS_ToBool(ctx, field(ctx, obj, "oneShot").get()) == 1;
  check(out.rate >= 0, "rate must be >= 0 (particles per second)");
  check(out.burst >= 0, "burst must be >= 0");
  check(out.max >= 1 && out.max <= core::kMaxParticles,
        "max must be 1.." + std::to_string(core::kMaxParticles) + ", got " + std::to_string(out.max));
  check(out.burst > 0 || !out.one_shot, "oneShot needs a burst > 0 (a one-shot emitter only emits its burst)");
}

}  // namespace

core::Blend parse_blend(JSContext* ctx, JSValueConst v, const std::string& path) {
  constexpr std::array<std::pair<const char*, core::Blend>, 3> kBlends{
      {{"alpha", core::Blend::Alpha}, {"add", core::Blend::Add}, {"multiply", core::Blend::Multiply}}};
  return choice(ctx, v, path, kBlends);
}

void parse_look(JSContext* ctx, JSValueConst obj, const std::string& path, core::ShapeSpec& shape) {
  if (has(ctx, obj, "tint")) shape.tint = color_arg(ctx, field(ctx, obj, "tint").get(), path + "tint");
  shape.alpha = number_or(ctx, obj, "alpha", path + "alpha", 1.0f);
  if (shape.alpha < 0 || shape.alpha > 1) throw core::Error(kSpawn + path + "alpha must be 0..1");
  if (has(ctx, obj, "blend")) shape.blend = parse_blend(ctx, field(ctx, obj, "blend").get(), path + "blend");
}

core::ShapeSpec parse_text_shape(JSContext* ctx, JSValueConst obj, ScriptState& s) {
  core::TextLook look;
  look.text = text_arg(ctx, field(ctx, obj, "text").get(), kSpawn + "shape.text");
  if (has(ctx, obj, "font")) look.font = string_arg(ctx, field(ctx, obj, "font").get(), kSpawn + "shape.font");
  look.size = number_or(ctx, obj, "size", "shape.size", 16.0f);
  if (!(look.size >= 1 && look.size <= 256)) throw core::Error(kSpawn + "shape.size must be 1..256 (pixels)");
  if (has(ctx, obj, "align")) look.align = parse_align(ctx, field(ctx, obj, "align").get());
  core::ShapeSpec shape{core::Rect{}, {255, 255, 255, 255}};
  if (has(ctx, obj, "color")) shape.fill = color_arg(ctx, field(ctx, obj, "color").get(), "shape.color");
  shape.text = std::move(look);
  lay_out(s, shape);
  return shape;
}

core::Layer parse_layer(JSContext* ctx, JSValueConst obj, const ScriptState& s) {
  if (!has(ctx, obj, "layer")) return {};
  const std::string name = string_arg(ctx, field(ctx, obj, "layer").get(), kSpawn + "layer");
  return named(s.layers, name, "layer", "config.layers");
}

core::EmitterSpec parse_emitter(JSContext* ctx, JSValueConst obj, const ScriptState& s) {
  if (!JS_IsObject(obj)) throw core::Error(kSpawn + "emitter must be an object { rate, burst, lifetime, ... }");
  core::EmitterSpec out;
  counts(ctx, obj, out);
  out.lifetime = span(ctx, obj, "lifetime", out.lifetime);
  check(out.lifetime.min > 0 && out.lifetime.max >= out.lifetime.min, "lifetime must be > 0 seconds, [min, max] with min <= max");
  out.speed = span(ctx, obj, "speed", out.speed);
  out.angle = span(ctx, obj, "angle", out.angle);
  out.size = span(ctx, obj, "size", out.size);
  if (has(ctx, obj, "gravity")) out.gravity = vec2_arg(ctx, field(ctx, obj, "gravity").get(), kSpawn + "emitter.gravity");
  colors(ctx, obj, out);
  out.look = particle_look(ctx, obj, s);
  parse_look(ctx, obj, kEmitter, out.look);
  return s.reduced_motion ? core::reduced_motion(out) : out;
}

}  // namespace alpha::script
