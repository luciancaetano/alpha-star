#pragma once

#include <vector>

#include "core/entity_spec.hpp"
#include "core/touch_pad.hpp"

namespace alpha::render {

// The game's touch sticks and buttons as screen overlay items, in game
// pixels: drawn over the world and its effects, under the game UI. Nothing
// unless the pad is active (mobile). Default look (no skin): grey translucent
// circles with smooth edges (sharp at any output size); skins draw their
// images. Opacity is the control's `opacity` at rest, 1 while touched.
struct TouchOverlay {
  std::vector<core::DrawItem> shapes;
  std::vector<core::TextItem> labels;
};
TouchOverlay touch_overlay(const core::TouchPad& pad);

}  // namespace alpha::render
