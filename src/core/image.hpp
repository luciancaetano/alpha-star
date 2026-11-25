#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace alpha::core {

// How a texture is sampled when drawn scaled or rotated: nearest (crisp
// pixel art), linear (smooth), sharp (pixel art grown by a whole factor, then
// linear: square texels with soft borders; core/image_filter).
enum class Filter : std::uint8_t { nearest, linear, sharp };

// Decoded image: tightly packed RGBA8, row-major, top row first.
struct Image {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> rgba;
  Filter filter = Filter::nearest;  // assets.images.<name>.filter
};

using ImagePtr = std::shared_ptr<const Image>;

// Integer pixel rectangle (source rects, tile cells, merged colliders).
struct Region {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  bool operator==(const Region&) const = default;
};

// What a game calls an image: a region of a decoded image, which is the
// whole file or its place in an atlas page (alpha export packs images).
struct Picture {
  ImagePtr image;
  Region region;
};
using PictureTable = std::map<std::string, Picture, std::less<>>;

}  // namespace alpha::core
