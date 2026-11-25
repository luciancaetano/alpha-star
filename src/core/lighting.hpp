#pragma once

#include <vector>

#include "core/camera.hpp"
#include "core/color.hpp"
#include "core/vec2.hpp"

// 2D lighting (config.lighting + spawn `light`): data and the pure
// world -> screen step; render::Compositor draws the lightmap.
namespace alpha::core {

// config.lighting: the light everywhere no light reaches (black = dark).
struct Lighting {
  Color ambient{40, 40, 60, 255};
};

// A light riding on an entity: `radius` world px (light fades to 0 at the
// edge), `color` times `intensity`.
struct LightSpec {
  float radius = 128.0f;
  Color color{255, 255, 255, 255};
  float intensity = 1.0f;
};

// Throws Error ("spawn: light.radius must be 1..4096, got ...") unless
// radius is 1..4096 and intensity 0..8.
LightSpec make_light(float radius, Color color, float intensity);

// One light as placed this frame (world position, its layer's parallax).
struct LightItem {
  Vec2 pos;
  LightSpec light;
  float parallax = 1.0f;
};

// A light on screen: center and radius in game pixels, color already times
// intensity (clamped to 255).
struct LightDisc {
  Vec2 center;
  float radius = 0.0f;
  Color color;
};

// The lights seen through `view` (each through its layer's parallax), in
// order; lights whose disc misses the screen are dropped.
std::vector<LightDisc> light_discs(const std::vector<LightItem>& lights, const View& view);

}  // namespace alpha::core
