#include "core/sheet.hpp"

#include <cmath>

namespace alpha::core {

namespace {

// Frames elapsed; the epsilon keeps sums of 1/60 steps from landing just
// short of a frame edge (6 steps at 10 fps must be frame 1).
std::size_t ticks(const Animation& anim, float time) {
  return static_cast<std::size_t>(std::floor(time * anim.fps + 1e-3f));
}

}  // namespace

int Sheet::frame_count() const {
  if (frame_w <= 0 || frame_h <= 0) return 0;
  return columns() * (picture.region.h / frame_h);
}

Region Sheet::frame(int index) const {
  const int cols = columns();
  return {picture.region.x + (index % cols) * frame_w, picture.region.y + (index / cols) * frame_h, frame_w,
          frame_h};
}

std::size_t frame_at(const Animation& anim, float time) {
  const std::size_t n = anim.frames.size();
  const std::size_t t = ticks(anim, time);
  if (anim.loop) return t % n;
  return t < n ? t : n - 1;
}

bool finished(const Animation& anim, float time) {
  return !anim.loop && ticks(anim, time) + 1 >= anim.frames.size();
}

}  // namespace alpha::core
