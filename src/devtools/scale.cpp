#include "devtools/scale.hpp"

#include <algorithm>

namespace alpha::devtools {

namespace {

constexpr float kReferenceSide = 1080.0f;  // ImGui's default size looks right on a 1080p screen
constexpr float kMaxScale = 4.0f;

}  // namespace

float tools_scale(int width, int height, float content_scale) {
  const float by_resolution = static_cast<float>(std::min(width, height)) / kReferenceSide;
  return std::clamp(std::max(by_resolution, content_scale), 1.0f, kMaxScale);
}

}  // namespace alpha::devtools
