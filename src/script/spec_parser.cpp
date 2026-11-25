#include "script/spec_parser.hpp"

#include <string>

#include "core/error.hpp"
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

// { image: name, size?: [w, h] }; size defaults to the image's own size.
core::ShapeSpec sprite(JSContext* ctx, JSValueConst obj, const ImageTable& images) {
  const std::string name = string_arg(ctx, field(ctx, obj, "image").get(), kPrefix + "shape.image");
  const auto it = images.find(name);
  if (it == images.end()) {
    std::string known;
    for (const auto& [key, img] : images) known += " " + key;
    throw core::Error(kPrefix + "shape.image \"" + name + "\" is not in assets.images; have:" +
                      (known.empty() ? " (none)" : known));
  }
  core::Vec2 size{static_cast<float>(it->second->width), static_cast<float>(it->second->height)};
  if (has(ctx, obj, "size")) size = vec2_arg(ctx, field(ctx, obj, "size").get(), kPrefix + "shape.size");
  return {core::Rect{size}, {}, {}, it->second};
}

core::ShapeSpec shape(JSContext* ctx, JSValueConst obj, const ImageTable& images) {
  if (has(ctx, obj, "image")) return sprite(ctx, obj, images);
  core::ShapeSpec out{geometry(ctx, obj, "shape"), {}, {}, {}};
  const Value fill = field(ctx, obj, "color");
  out.fill = color(ctx, fill.get(), "shape.color");
  if (has(ctx, obj, "outline")) {
    const Value outline = field(ctx, obj, "outline");
    out.outline = color(ctx, outline.get(), "shape.outline");
  }
  return out;
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
                                   const ImageTable& images) {
  if (!JS_IsObject(spec)) throw core::Error(kPrefix + "expected an entity spec object");
  core::EntitySpec out;
  out.tags = tag_set(ctx, spec, tags);
  const Value pos = field(ctx, spec, "pos");
  out.pos = vec2_arg(ctx, pos.get(), kPrefix + "pos");
  out.rotation = static_cast<float>(optional_number(ctx, spec, "rotation"));
  out.z = static_cast<int>(optional_number(ctx, spec, "z"));
  if (has(ctx, spec, "shape")) out.shape = shape(ctx, field(ctx, spec, "shape").get(), images);
  if (has(ctx, spec, "body")) out.body = body(ctx, field(ctx, spec, "body").get());
  return out;
}

}  // namespace alpha::script
