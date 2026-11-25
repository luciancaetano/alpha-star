#pragma once

#include "core/vec2.hpp"

// How the game's logical pixels land on the output (window pixels, high-DPI
// included): output = logical * scale + offset. Text is rasterized at the
// output size and placed on whole output pixels, so nothing is resampled.
namespace alpha::core {

struct PixelGrid {
  Vec2 scale{1.0f, 1.0f};  // output pixels per logical pixel
  Vec2 offset;             // where logical (0, 0) lands (letterbox bars)
};

// The grid of a logical `size` shown in the output rect at `origin` with `shown` size.
PixelGrid pixel_grid(Vec2 origin, Vec2 shown, Vec2 size);
// The logical point nearest to `p` that lands on a whole output pixel.
Vec2 snap(const PixelGrid& grid, Vec2 p);

}  // namespace alpha::core
