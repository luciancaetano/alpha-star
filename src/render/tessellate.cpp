#include "render/tessellate.hpp"

#include "core/camera.hpp"
#include "core/particles.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace alpha::render {

namespace {

constexpr float kMaxError = 0.3f;  // output pixels between the polygon and the true circle
constexpr int kMinSegments = 12;
constexpr int kMaxSegments = 128;

struct Placement {
  core::Vec2 pos;
  float degrees;
  float grow;  // extra size on every side
};

using Rim = std::vector<core::Vec2>;

// The shape's edge, moved out by `d` world units (in when negative), in order.
void rim(const core::Geometry& g, const Placement& p, float d, float pixel, Rim& out) {
  out.clear();
  if (const auto* c = std::get_if<core::Circle>(&g)) {
    const float r = c->radius + p.grow;
    const int n = circle_segments(r / pixel);
    const float step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(n);
    const float at = std::max(0.0f, r + d);
    for (int i = 0; i < n; ++i) {
      const float a = step * static_cast<float>(i);
      out.push_back(p.pos + core::Vec2{std::cos(a) * at, std::sin(a) * at});
    }
    return;
  }
  const core::Vec2 size = std::get<core::Rect>(g).size;
  const core::Vec2 h{std::max(0.0f, size.x * 0.5f + p.grow + d), std::max(0.0f, size.y * 0.5f + p.grow + d)};
  for (const core::Vec2 corner : {core::Vec2{-h.x, -h.y}, core::Vec2{h.x, -h.y}, core::Vec2{h.x, h.y}, core::Vec2{-h.x, h.y}}) {
    out.push_back(p.pos + core::rotated(corner, p.degrees));
  }
}

// A rect's 4 corners as two triangles; a circle's rim as a fan from its center.
void fill(core::Vec2 center, const Rim& pts, core::Color color, std::vector<Vertex>& out) {
  if (pts.size() == 4) {
    for (const std::size_t i : {0u, 1u, 2u, 0u, 2u, 3u}) out.push_back({pts[i], color, {}});
    return;
  }
  for (std::size_t i = 0; i < pts.size(); ++i) {
    out.push_back({center, color, {}});
    out.push_back({pts[i], color, {}});
    out.push_back({pts[(i + 1) % pts.size()], color, {}});
  }
}

// Quads from `inner` (color) to `outer` (same color at alpha 0).
void ring(const Rim& inner, const Rim& outer, core::Color color, std::vector<Vertex>& out) {
  core::Color clear = color;
  clear.a = 0;
  for (std::size_t i = 0; i < inner.size(); ++i) {
    const std::size_t j = (i + 1) % inner.size();
    const std::array<Vertex, 4> q{Vertex{inner[i], color, {}}, Vertex{outer[i], clear, {}},
                                  Vertex{outer[j], clear, {}}, Vertex{inner[j], color, {}}};
    for (const std::size_t k : {0u, 1u, 2u, 0u, 2u, 3u}) out.push_back(q[k]);
  }
}

void shape(const core::ShapeSpec& s, const Placement& p, core::Color color, float pixel, std::vector<Vertex>& out) {
  Rim inner;
  if (!s.smooth) {
    rim(s.geometry, p, 0.0f, pixel, inner);
    return fill(p.pos, inner, color, out);
  }
  Rim outer;
  rim(s.geometry, p, -0.5f * pixel, pixel, inner);
  rim(s.geometry, p, 0.5f * pixel, pixel, outer);
  fill(p.pos, inner, color, out);
  ring(inner, outer, color, out);
}

}  // namespace

int circle_segments(float radius_px) {
  if (radius_px <= kMaxError) return kMinSegments;
  const float n = std::numbers::pi_v<float> / std::acos(1.0f - kMaxError / radius_px);
  return std::clamp(static_cast<int>(std::ceil(n)), kMinSegments, kMaxSegments);
}

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

void tessellate(const core::DrawItem& item, std::vector<Vertex>& out, float pixel) {
  const core::ShapeSpec& s = item.shape;
  if (s.outline) {
    shape(s, {item.pos, item.rotation, kOutlineWidth}, core::shade(*s.outline, s.tint, s.alpha), pixel, out);
  }
  shape(s, {item.pos, item.rotation, 0.0f}, core::shade(s.fill, s.tint, s.alpha), pixel, out);
}

std::vector<Vertex> tessellate(std::span<const core::DrawItem> items, float pixel) {
  std::vector<Vertex> out;
  for (const core::DrawItem& item : items) tessellate(item, out, pixel);
  return out;
}

}  // namespace alpha::render
