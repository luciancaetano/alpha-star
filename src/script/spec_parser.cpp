#include "script/spec_parser.hpp"

#include <string>
#include <utility>

#include "core/error.hpp"
#include "core/index_of.hpp"
#include "script/body_parser.hpp"
#include "script/effect_parser.hpp"
#include "script/js_value.hpp"
#include "script/spec_fields.hpp"

namespace alpha::script {

namespace {

const std::string& kPrefix = kSpawn;

core::Color color(JSContext* ctx, JSValueConst obj, const std::string& path) { return color_arg(ctx, obj, path); }

bool flag(JSContext* ctx, JSValueConst obj, const char* name) {
  return JS_ToBool(ctx, field(ctx, obj, name).get()) == 1;
}

// Shared by images and sheets: size (default: the source rect's size) and flips.
core::ShapeSpec picture_shape(JSContext* ctx, JSValueConst obj, const core::ImagePtr& image, core::Region src) {
  core::Vec2 size{static_cast<float>(src.w), static_cast<float>(src.h)};
  if (has(ctx, obj, "size")) size = vec2_arg(ctx, field(ctx, obj, "size").get(), kPrefix + "shape.size");
  core::ShapeSpec out{core::Rect{size}, {}, {}, image};
  out.src = src;
  out.flip_x = flag(ctx, obj, "flipX");
  out.flip_y = flag(ctx, obj, "flipY");
  return out;
}

// { image: name, size?, flipX?, flipY? }
core::ShapeSpec image(JSContext* ctx, JSValueConst obj, const ScriptState& assets) {
  const std::string name = string_arg(ctx, field(ctx, obj, "image").get(), kPrefix + "shape.image");
  const core::Picture pic = named(assets.images, name, "shape.image", "assets.images");
  return picture_shape(ctx, obj, pic.image, pic.region);
}

// { sheet: name, animation?: name, size?, flipX?, flipY? }: starts at frame 0.
std::pair<core::ShapeSpec, core::AnimationSpec> sheet(JSContext* ctx, JSValueConst obj, const ScriptState& assets) {
  const std::string name = string_arg(ctx, field(ctx, obj, "sheet").get(), kPrefix + "shape.sheet");
  const core::SheetPtr sheet = named(assets.sheets, name, "shape.sheet", "assets.sheets");
  std::string anim;
  if (has(ctx, obj, "animation")) {
    anim = string_arg(ctx, field(ctx, obj, "animation").get(), kPrefix + "shape.animation");
    named(sheet->animations, anim, "shape.animation", "assets.sheets." + name + ".animations");
  }
  return {picture_shape(ctx, obj, sheet->picture.image, sheet->frame(0)), {sheet, anim}};
}

core::ShapeSpec filled(JSContext* ctx, JSValueConst obj) {
  core::ShapeSpec out{parse_geometry(ctx, obj, "shape"), {}, {}, {}};
  const Value fill = field(ctx, obj, "color");
  out.fill = color(ctx, fill.get(), "shape.color");
  if (has(ctx, obj, "outline")) {
    const Value outline = field(ctx, obj, "outline");
    out.outline = color(ctx, outline.get(), "shape.outline");
  }
  return out;
}

void shape(JSContext* ctx, JSValueConst obj, ScriptState& assets, core::EntitySpec& out) {
  if (!JS_IsObject(obj)) throw core::Error(kPrefix + "shape must be an object");
  if (has(ctx, obj, "text")) {
    out.shape = parse_text_shape(ctx, obj, assets);
  } else if (has(ctx, obj, "sheet")) {
    auto [look, anim] = sheet(ctx, obj, assets);
    look.animation = std::move(anim);
    out.shape = std::move(look);
  } else if (has(ctx, obj, "image")) {
    out.shape = image(ctx, obj, assets);
  } else {
    out.shape = filled(ctx, obj);
  }
  parse_look(ctx, obj, "shape.", *out.shape);
}

core::TagSet tag_set(JSContext* ctx, JSValueConst obj, core::TagRegistry& tags) {
  const Value list = field(ctx, obj, "tags");
  core::TagSet set;
  for (const std::string& name : strings_arg(ctx, list.get(), kPrefix + "tags")) {
    set.set(tags.intern(name));
  }
  return set;
}

double optional_number(JSContext* ctx, JSValueConst obj, const char* name) {
  if (!has(ctx, obj, name)) return 0.0;
  const Value v = field(ctx, obj, name);
  return number_arg(ctx, v.get(), kPrefix + name);
}

}  // namespace

core::EntitySpec parse_entity_spec(JSContext* ctx, JSValueConst spec, ScriptState& s) {
  if (!JS_IsObject(spec)) throw core::Error(kPrefix + "expected an entity spec object");
  core::EntitySpec out;
  out.tags = tag_set(ctx, spec, s.tags);
  const Value pos = field(ctx, spec, "pos");
  out.pos = vec2_arg(ctx, pos.get(), kPrefix + "pos");
  out.rotation = static_cast<float>(optional_number(ctx, spec, "rotation"));
  out.z = static_cast<int>(optional_number(ctx, spec, "z"));
  out.layer = parse_layer(ctx, spec, s);
  if (has(ctx, spec, "shape")) shape(ctx, field(ctx, spec, "shape").get(), s, out);
  if (has(ctx, spec, "body")) out.body = parse_body(ctx, field(ctx, spec, "body").get(), s);
  if (has(ctx, spec, "emitter")) out.emitter = parse_emitter(ctx, field(ctx, spec, "emitter").get(), s);
  return out;
}

}  // namespace alpha::script
