#include "script/body_parser.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include "core/error.hpp"
#include "script/js_value.hpp"
#include "script/spec_fields.hpp"

namespace alpha::script {

namespace {

constexpr std::array<std::pair<const char*, core::BodyType>, 3> kTypes{{
    {"static", core::BodyType::Static}, {"kinematic", core::BodyType::Kinematic}, {"dynamic", core::BodyType::Dynamic}}};

bool flag(JSContext* ctx, JSValueConst obj, const char* name) { return JS_ToBool(ctx, field(ctx, obj, name).get()) == 1; }

core::ColliderSpec collider(JSContext* ctx, JSValueConst obj) {
  if (!JS_IsObject(obj)) throw core::Error(kSpawn + "body.collider is required: { circle: r } or { rect: [w, h] }");
  core::ColliderSpec out{parse_geometry(ctx, obj, "body.collider"), flag(ctx, obj, "sensor")};
  out.material.friction = number_or(ctx, obj, "friction", "body.collider.friction", out.material.friction);
  out.material.restitution = number_or(ctx, obj, "restitution", "body.collider.restitution", 0.0f);
  out.material.density = number_or(ctx, obj, "density", "body.collider.density", 1.0f);
  out.one_way = flag(ctx, obj, "oneWay");
  if (out.material.friction < 0 || out.material.restitution < 0 || out.material.density < 0) {
    throw core::Error(kSpawn + "body.collider friction, restitution and density must be >= 0");
  }
  return out;
}

core::CollisionFilter filter(JSContext* ctx, JSValueConst obj, const ScriptState& s) {
  core::CollisionFilter out;
  if (has(ctx, obj, "category")) {
    const std::string name = string_arg(ctx, field(ctx, obj, "category").get(), kSpawn + "body.category");
    out.category = category_bits(s, {name}, "spawn: body.category");
  }
  if (has(ctx, obj, "collidesWith")) {
    const auto names = strings_arg(ctx, field(ctx, obj, "collidesWith").get(), kSpawn + "body.collidesWith");
    out.mask = category_bits(s, names, "spawn: body.collidesWith");
  }
  return out;
}

}  // namespace

core::Geometry parse_geometry(JSContext* ctx, JSValueConst obj, const std::string& path) {
  if (has(ctx, obj, "circle")) {
    return core::Circle{static_cast<float>(number_arg(ctx, field(ctx, obj, "circle").get(), kSpawn + path + ".circle"))};
  }
  if (has(ctx, obj, "rect")) return core::Rect{vec2_arg(ctx, field(ctx, obj, "rect").get(), kSpawn + path + ".rect")};
  throw core::Error(kSpawn + path + " needs { circle: radius } or { rect: [width, height] }");
}

std::uint64_t category_bits(const ScriptState& s, const std::vector<std::string>& names, const std::string& what) {
  std::uint64_t bits = 0;
  for (const std::string& name : names) {
    const auto it = std::find(s.categories.begin(), s.categories.end(), name);
    if (it == s.categories.end()) {
      std::string have;
      for (const std::string& c : s.categories) have += (have.empty() ? "" : " ") + c;
      throw core::Error(what + " \"" + name + "\" is not in config.collision.categories; have: " +
                        (have.empty() ? "(none)" : have));
    }
    bits |= std::uint64_t{1} << (1 + (it - s.categories.begin()));
  }
  return bits;
}

core::BodySpec parse_body(JSContext* ctx, JSValueConst obj, const ScriptState& s) {
  if (!JS_IsObject(obj)) throw core::Error(kSpawn + "body must be an object { type, collider }");
  core::BodySpec out{choice(ctx, field(ctx, obj, "type").get(), "body.type", kTypes),
                     collider(ctx, field(ctx, obj, "collider").get()), {}};
  if (has(ctx, obj, "velocity")) out.velocity = vec2_arg(ctx, field(ctx, obj, "velocity").get(), kSpawn + "body.velocity");
  out.fixed_rotation = flag(ctx, obj, "fixedRotation");
  out.gravity_scale = number_or(ctx, obj, "gravityScale", "body.gravityScale", 1.0f);
  out.bullet = flag(ctx, obj, "bullet");
  out.filter = filter(ctx, obj, s);
  return out;
}

}  // namespace alpha::script
