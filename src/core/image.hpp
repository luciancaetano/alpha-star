#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace alpha::core {

// Decoded image: tightly packed RGBA8, row-major, top row first.
struct Image {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> rgba;
};

using ImagePtr = std::shared_ptr<const Image>;

}  // namespace alpha::core
