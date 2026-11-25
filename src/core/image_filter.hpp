#pragma once

#include "core/image.hpp"

namespace alpha::core {

// Filter::sharp ("sharp bilinear"): the image is grown by a whole factor with
// nearest sampling, then drawn with linear filtering, so texels stay square
// blocks at any scale or rotation and only their borders blend (about one
// screen pixel while the on-screen scale is <= the factor).
inline constexpr int kSharpScale = 4;        // the factor, when the texture fits
inline constexpr int kSharpMaxSide = 4096;   // largest grown texture side

// Whole factor in [1, kSharpScale] with factor * max(w, h) <= kSharpMaxSide.
int sharp_factor(int width, int height);

// `in` with every texel repeated into a k x k block (k >= 1).
Image upscaled(const Image& in, int k);

}  // namespace alpha::core
