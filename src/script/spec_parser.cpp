#include "script/spec_parser.hpp"

#include <string>
#include <utility>

#include "core/error.hpp"
#include "core/index_of.hpp"
#include "script/js_value.hpp"

namespace alpha::script {

namespace {

const std::string kPrefix = "spawn: ";

bool has(JSContext* ctx, JSValueConst obj, const char* name) {
  const Value v(ctx, JS_GetPropertyStr(ctx, obj, name));
  return !JS_IsUndefined(v.get());
}

Value field(JSContext* ctx, JSValueConst obj, const char* name) {
  return {ctx, JS_GetPropertyStr(ctx, obj, name)};
}

core::Color color(JSContext* ctx, JSValueConst obj, const std::string& path) {
  const std::string text = string_arg(ctx, obj, kPrefix + path);
  const auto parsed = core::parse_color(text);
  if (!parsed) throw core::Error(kPrefix + path + " must be a color like \"#73bf2e\", got \"" + text + "\"");
  return *parsed;
}

// { circle: r } or { rect: [w, h] }
core::Geometry geometry(JSContext* ctx, JSValueConst obj, const std::string& path) {
  if (has(ctx, obj, "circle")) {
    const Value r = field(ctx, obj, "circle");
    return core::Circle{static_cast<float>(number_arg(ctx, r.get(), kPrefix + path + ".circle"))};
  }
  if (has(ctx, obj, "rect")) {
    const Value size = field(ctx, obj, "rect");
    return core::Rect{vec2_arg(ctx, size.get(), kPrefix + path + ".rect")};
  }
  throw core::Error(kPrefix + path + " needs { circle: radius } or { rect: [width, height] }");
}

// assets.<kind>[name], or an error listing what the game declared.
template <typename Table>
const typename Table::mapped_type& named(const Table& table, const std::string& name, const std::string& field,
                                         const char* kind) {
  const auto it = table.find(name);
  if (it != table.end()) return it->second;
  const std::string known = core::keys_of(table, " ");
  throw core::Error(kPrefix + field + " \"" + name + "\" is not in assets." + kind + "; have: " +
                    (known.empty() ? "(none)" : known));
}

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
core::ShapeSpec image(JSContext* ctx, JSValueConst obj, const SpriteAssets& assets) {
  const std::string name = string_arg(ctx, field(ctx, obj, "image").get(), kPrefix + "shape.image");
  const core::Picture pic = named(assets.images, name, "shape.image", "images");
  return picture_shape(ctx, obj, pic.image, pic.region);
}

// { sheet: name, animation?: name, size?, flipX?, flipY? }: starts at frame 0.
std::pair<core::ShapeSpec, core::AnimationSpec> sheet(JSContext* ctx, JSValueConst obj, const SpriteAssets& assets) {
  const std::string name = string_arg(ctx, field(ctx, obj, "sheet").get(), kPrefix + "shape.sheet");
  const core::SheetPtr sheet = named(assets.sheets, name, "shape.sheet", "sheets");
  std::string anim;
  if (has(ctx, obj, "animation")) {
    anim = string_arg(ctx, field(ctx, obj, "animation").get(), kPrefix + "shape.animation");
    named(sheet->animations, anim, "shape.animation", ("sheets." + name + ".animations").c_str());
  }
  return {picture_shape(ctx, obj, sheet->picture.image, sheet->frame(0)), {sheet, anim}};
}

core::ShapeSpec filled(JSContext* ctx, JSValueConst obj) {
  core::ShapeSpec out{geometry(ctx, obj, "shape"), {}, {}, {}};
  const Value fill = field(ctx, obj, "color");
  out.fill = color(ctx, fill.get(), "shape.color");
  if (has(ctx, obj, "outline")) {
    const Value outline = field(ctx, obj, "outline");
    out.outline = color(ctx, outline.get(), "shape.outline");
  }
  return out;
}

void shape(JSContext* ctx, JSValueConst obj, const SpriteAssets& assets, core::EntitySpec& out) {
  if (!JS_IsObject(obj)) throw core::Error(kPrefix + "shape must be an object");
  if (has(ctx, obj, "sheet")) {
    auto [look, anim] = sheet(ctx, obj, assets);
    look.animation = std::move(anim);
    out.shape = std::move(look);
  } else if (has(ctx, obj, "image")) {
    out.shape = image(ctx, obj, assets);
  } else {
    out.shape = filled(ctx, obj);
  }
}

core::BodyType body_type(JSContext* ctx, JSValueConst obj) {
  const Value type = field(ctx, obj, "type");
  const std::string name = string_arg(ctx, type.get(), kPrefix + "body.type");
  if (name == "static") return core::BodyType::Static;
  if (name == "kinematic") return core::BodyType::Kinematic;
  if (name == "dynamic") return core::BodyType::Dynamic;
  throw core::Error(kPrefix + "body.type must be \"static\", \"kinematic\" or \"dynamic\", got \"" + name + "\"");
}

core::BodySpec body(JSContext* ctx, JSValueConst obj) {
  core::BodySpec out;
  out.type = body_type(ctx, obj);
  const Value collider = field(ctx, obj, "collider");
  if (!JS_IsObject(collider.get())) throw core::Error(kPrefix + "body.collider is required");
  out.collider.geometry = geometry(ctx, collider.get(), "body.collider");
  const Value sensor = field(ctx, collider.get(), "sensor");
  out.collider.sensor = JS_ToBool(ctx, sensor.get()) == 1;
  if (has(ctx, obj, "velocity")) {
    const Value v = field(ctx, obj, "velocity");
    out.velocity = vec2_arg(ctx, v.get(), kPrefix + "body.velocity");
  }
  return out;
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

core::EntitySpec parse_entity_spec(JSContext* ctx, JSValueConst spec, core::TagRegistry& tags,
                                   const SpriteAssets& assets) {
  if (!JS_IsObject(spec)) throw core::Error(kPrefix + "expected an entity spec object");
  core::EntitySpec out;
  out.tags = tag_set(ctx, spec, tags);
  const Value pos = field(ctx, spec, "pos");
  out.pos = vec2_arg(ctx, pos.get(), kPrefix + "pos");
  out.rotation = static_cast<float>(optional_number(ctx, spec, "rotation"));
  out.z = static_cast<int>(optional_number(ctx, spec, "z"));
  if (has(ctx, spec, "shape")) shape(ctx, field(ctx, spec, "shape").get(), assets, out);
  if (has(ctx, spec, "body")) out.body = body(ctx, field(ctx, spec, "body").get());
  return out;
}

}  // namespace alpha::script
