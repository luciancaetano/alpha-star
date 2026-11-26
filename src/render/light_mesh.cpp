#include "render/light_mesh.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace alpha::render {

namespace {

constexpr int kLightSegments = 48;
constexpr int kVignetteCells = 12;
constexpr float kTau = 6.2831853f;

core::Color dimmed(core::Color c, float k) {
  core::Color out = core::scaled(c, k);
  out.a = 255;
  return out;
}

core::Color black(float alpha) { return {0, 0, 0, static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f + 0.5f)}; }

void quad(const std::vector<Vertex>& corners, std::vector<Vertex>& out) {  // corners: a b c d around
  out.insert(out.end(), {corners[0], corners[1], corners[2], corners[0], corners[2], corners[3]});
}

}  // namespace

void light_triangles(const core::LightDisc& disc, std::vector<Vertex>& out) {
  const core::Color mid = dimmed(disc.color, 0.35f);
  const core::Color edge{0, 0, 0, 255};
  for (int i = 0; i < kLightSegments; ++i) {
    const float a0 = kTau * static_cast<float>(i) / kLightSegments;
    const float a1 = kTau * static_cast<float>(i + 1) / kLightSegments;
    const core::Vec2 d0{std::cos(a0) * disc.radius, std::sin(a0) * disc.radius};
    const core::Vec2 d1{std::cos(a1) * disc.radius, std::sin(a1) * disc.radius};
    const Vertex m0{disc.center + d0 * 0.5f, mid};
    const Vertex m1{disc.center + d1 * 0.5f, mid};
    out.insert(out.end(), {Vertex{disc.center, disc.color}, m0, m1});
    quad({m0, Vertex{disc.center + d0, edge}, Vertex{disc.center + d1, edge}, m1}, out);
  }
}

void vignette_triangles(core::Vec2 size, float strength, float radius, std::vector<Vertex>& out) {
  const auto at = [&](int i, int j) {
    const core::Vec2 p{size.x * static_cast<float>(i) / kVignetteCells, size.y * static_cast<float>(j) / kVignetteCells};
    const float dx = p.x / size.x * 2.0f - 1.0f;
    const float dy = p.y / size.y * 2.0f - 1.0f;
    const float d = std::sqrt(dx * dx + dy * dy) / std::sqrt(2.0f);
    const float t = std::clamp((d - radius) / std::max(1.0f - radius, 0.001f), 0.0f, 1.0f);
    return Vertex{p, black(strength * t * t)};
  };
  for (int j = 0; j < kVignetteCells; ++j) {
    for (int i = 0; i < kVignetteCells; ++i) quad({at(i, j), at(i + 1, j), at(i + 1, j + 1), at(i, j + 1)}, out);
  }
}

void scanline_triangles(core::Vec2 size, float strength, std::vector<Vertex>& out) {
  const core::Color c = black(strength);
  for (float y = 1.0f; y < size.y; y += 2.0f) {
    quad({{{0, y}, c}, {{size.x, y}, c}, {{size.x, y + 1}, c}, {{0, y + 1}, c}}, out);
  }
}

}  // namespace alpha::render
