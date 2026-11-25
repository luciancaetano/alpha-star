#pragma once

#include <cstddef>
#include <functional>
#include <list>

#include "core/glyph_layout.hpp"

namespace alpha::render {

// World-text atlases by (font, pixel size), most recently used kept: text is
// rasterized at the output size (window scale x camera zoom), so a zoom tween
// or a resize asks for new sizes; the least recently used beyond `capacity`
// is dropped and `evicted` is told its image (to free the texture).
class FontCache {
 public:
  using Evicted = std::function<void(const core::Image*)>;
  FontCache(std::size_t capacity, Evicted evicted);

  core::FontAtlasPtr get(const core::FontSourcePtr& font, int pixels);
  std::size_t size() const { return entries_.size(); }

 private:
  struct Entry {
    core::FontSourcePtr font;  // held, so its address is never reused while cached
    int pixels = 0;
    core::FontAtlasPtr atlas;
  };
  std::size_t capacity_;
  Evicted evicted_;
  std::list<Entry> entries_;  // most recent first
};

}  // namespace alpha::render
