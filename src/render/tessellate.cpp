#include "render/tessellate.hpp"

#include <array>
#include <cmath>
#include <numbers>

namespace alpha::render {

namespace {

constexpr float kDegToRad = std::numbers::pi_v<float> / 180.0f;

struct Placement {
  core::Vec2 pos;
  float radians;
  float grow;  // extra size on every side
};

core::Vec2 rotate(core::Vec2 v, float radians) {
  const float c = std::cos(radians);
  const float s = std::sin(radians);
  return {v.x * c - v.y * s, v.x * s + v.y * c};
}

void circle(float radius, const Placement& p, core::Color color, std::vector<Vertex>& out) {
  const float r = radius + p.grow;
  const float step = 2.0f * std::numbers::pi_v<float> / kCircleSegments;
  for (int i = 0; i < kCircleSegments; ++i) {
    const float a0 = step * static_cast<float>(i);
    const float a1 = step * static_cast<float>(i + 1);
    out.push_back({p.pos, color});
    out.push_back({p.pos + core::Vec2{std::cos(a0) * r, std::sin(a0) * r}, color});
    out.push_back({p.pos + core::Vec2{std::cos(a1) * r, std::sin(a1) * r}, color});
  }
}

void rect(core::Vec2 size, const Placement& p, core::Color color, std::vector<Vertex>& out) {
  const core::Vec2 h{size.x * 0.5f + p.grow, size.y * 0.5f + p.grow};
  const std::array<core::Vec2, 4> corners{
      core::Vec2{-h.x, -h.y}, core::Vec2{h.x, -h.y}, core::Vec2{h.x, h.y}, core::Vec2{-h.x, h.y}};
  std::array<Vertex, 4> v{};
  for (std::size_t i = 0; i < corners.size(); ++i) {
    v[i] = {p.pos + rotate(corners[i], p.radians), color};
  }
  for (const std::size_t i : {0u, 1u, 2u, 0u, 2u, 3u}) out.push_back(v[i]);
}

void shape(const core::Geometry& g, const Placement& p, core::Color color,
           std::vector<Vertex>& out) {
  if (const auto* c = std::get_if<core::Circle>(&g)) {
    circle(c->radius, p, color, out);
  } else {
    rect(std::get<core::Rect>(g).size, p, color, out);
  }
}

}  // namespace

std::vector<Vertex> tessellate(std::span<const core::DrawItem> items) {
  std::vector<Vertex> out;
  for (const core::DrawItem& item : items) {
    const float radians = item.rotation * kDegToRad;
    if (item.shape.outline) {
      shape(item.shape.geometry, {item.pos, radians, kOutlineWidth}, *item.shape.outline, out);
    }
    shape(item.shape.geometry, {item.pos, radians, 0.0f}, item.shape.fill, out);
  }
  return out;
}

}  // namespace alpha::render
