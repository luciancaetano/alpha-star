#include "core/image_filter.hpp"

#include <algorithm>
#include <cstddef>

namespace alpha::core {

int sharp_factor(int width, int height) {
  const int side = std::max({width, height, 1});
  return std::clamp(kSharpMaxSide / side, 1, kSharpScale);
}

Image upscaled(const Image& in, int k) {
  Image out{in.width * k, in.height * k, {}, in.filter};
  out.rgba.resize(static_cast<std::size_t>(out.width) * static_cast<std::size_t>(out.height) * 4);
  for (int y = 0; y < out.height; ++y) {
    const auto row = static_cast<std::size_t>(y / k) * static_cast<std::size_t>(in.width);
    for (int x = 0; x < out.width; ++x) {
      const auto from = (row + static_cast<std::size_t>(x / k)) * 4;
      const auto to = (static_cast<std::size_t>(y) * static_cast<std::size_t>(out.width) + static_cast<std::size_t>(x)) * 4;
      std::copy_n(in.rgba.begin() + static_cast<std::ptrdiff_t>(from), 4, out.rgba.begin() + static_cast<std::ptrdiff_t>(to));
    }
  }
  return out;
}

}  // namespace alpha::core
