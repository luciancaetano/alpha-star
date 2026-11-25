#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/image.hpp"
#include "core/vec2.hpp"

// Sprite sheets: a grid of equal frames inside a picture, and named
// animations over frame indices. Pure data + timing; advanced on the fixed step.
namespace alpha::core {

struct Animation {
  std::vector<int> frames;  // frame indices, row-major in the sheet
  float fps = 10.0f;        // > 0
  bool loop = true;
};

struct Sheet {
  Picture picture;           // the sheet's pixels (possibly a region of an atlas page)
  int frame_w = 0;
  int frame_h = 0;
  std::map<std::string, Animation, std::less<>> animations;

  int columns() const { return frame_w > 0 ? picture.region.w / frame_w : 0; }
  int frame_count() const;
  // Source rect of frame `index` in the picture's image.
  Region frame(int index) const;
};

using SheetPtr = std::shared_ptr<const Sheet>;
using SheetTable = std::map<std::string, SheetPtr, std::less<>>;

// Position in anim.frames `time` seconds after the animation started: loops
// wrap, others hold their last frame.
std::size_t frame_at(const Animation& anim, float time);
// A non-looping animation that reached its last frame.
bool finished(const Animation& anim, float time);

}  // namespace alpha::core
