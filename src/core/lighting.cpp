#include "core/lighting.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

#include "core/error.hpp"

namespace alpha::core {

namespace {

std::uint8_t scaled(std::uint8_t c, float k) { return static_cast<std::uint8_t>(std::min(255.0f, c * k + 0.5f)); }

}  // namespace

LightSpec make_light(float radius, Color color, float intensity) {
  if (!(radius >= 1.0f && radius <= 4096.0f)) throw Error("spawn: light.radius must be 1..4096, got " + std::to_string(radius));
  if (!(intensity >= 0.0f && intensity <= 8.0f)) {
    throw Error("spawn: light.intensity must be 0..8, got " + std::to_string(intensity));
  }
  return {radius, color, intensity};
}

std::vector<LightDisc> light_discs(const std::vector<LightItem>& lights, const View& view) {
  std::vector<LightDisc> out;
  for (const LightItem& l : lights) {
    const View seen = parallax_view(view, l.parallax);
    const Vec2 c = to_screen(seen, l.pos);
    const float r = l.light.radius * seen.zoom;
    if (c.x + r < 0 || c.y + r < 0 || c.x - r > view.size.x || c.y - r > view.size.y) continue;
    const float k = l.light.intensity * (l.light.color.a / 255.0f);
    const Color col = l.light.color;
    out.push_back({c, r, {scaled(col.r, k), scaled(col.g, k), scaled(col.b, k), 255}});
  }
  return out;
}

}  // namespace alpha::core
