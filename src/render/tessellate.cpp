#include "render/tessellate.hpp"

#include "core/camera.hpp"
#include "core/particles.hpp"

#include <array>
#include <cmath>
#include <numbers>

namespace alpha::render {

namespace {

struct Placement {
  core::Vec2 pos;
  float degrees;
  float grow;  // extra size on every side
};


void circle(float radius, const Placement& p, core::Color color, std::vector<Vertex>& out) {
  fan(p.pos, radius + p.grow, kCircleSegments, color, out);
}

void rect(core::Vec2 size, const Placement& p, core::Color color, std::vector<Vertex>& out) {
  const core::Vec2 h{size.x * 0.5f + p.grow, size.y * 0.5f + p.grow};
  const std::array<core::Vec2, 4> corners{
      core::Vec2{-h.x, -h.y}, core::Vec2{h.x, -h.y}, core::Vec2{h.x, h.y}, core::Vec2{-h.x, h.y}};
  std::array<Vertex, 4> v{};
  for (std::size_t i = 0; i < corners.size(); ++i) {
    v[i] = {p.pos + core::rotated(corners[i], p.degrees), color, {}};
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

void fan(core::Vec2 center, float radius, int segments, core::Color color, std::vector<Vertex>& out) {
  const float step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(segments);
  for (int i = 0; i < segments; ++i) {
    const float a0 = step * static_cast<float>(i);
    const float a1 = a0 + step;
    out.push_back({center, color, {}});
    out.push_back({center + core::Vec2{std::cos(a0) * radius, std::sin(a0) * radius}, color, {}});
    out.push_back({center + core::Vec2{std::cos(a1) * radius, std::sin(a1) * radius}, color, {}});
  }
}

void tessellate(const core::DrawItem& item, std::vector<Vertex>& out) {
  const core::ShapeSpec& s = item.shape;
  if (s.outline) {
    shape(s.geometry, {item.pos, item.rotation, kOutlineWidth}, core::shade(*s.outline, s.tint, s.alpha), out);
  }
  shape(s.geometry, {item.pos, item.rotation, 0.0f}, core::shade(s.fill, s.tint, s.alpha), out);
}

std::vector<Vertex> tessellate(std::span<const core::DrawItem> items) {
  std::vector<Vertex> out;
  for (const core::DrawItem& item : items) tessellate(item, out);
  return out;
}

}  // namespace alpha::render
