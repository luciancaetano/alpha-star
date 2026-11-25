#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace alpha::core {

// Decoded image: tightly packed RGBA8, row-major, top row first.
struct Image {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> rgba;
  bool smooth = false;  // linear filtering when scaled (pixelMode: false); nearest otherwise
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
