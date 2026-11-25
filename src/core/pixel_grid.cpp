#include "core/pixel_grid.hpp"

#include <cmath>

namespace alpha::core {

PixelGrid pixel_grid(Vec2 origin, Vec2 shown, Vec2 size) {
  if (shown.x <= 0 || shown.y <= 0 || size.x <= 0 || size.y <= 0) return {};
  return {{shown.x / size.x, shown.y / size.y}, origin};
}

Vec2 snap(const PixelGrid& grid, Vec2 p) {
  const auto axis = [](float v, float scale, float offset) { return (std::round(v * scale + offset) - offset) / scale; };
  return {axis(p.x, grid.scale.x, grid.offset.x), axis(p.y, grid.scale.y, grid.offset.y)};
}

}  // namespace alpha::core
