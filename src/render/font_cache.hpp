#pragma once

#include <cstddef>
#include <functional>
#include <list>

#include "core/glyph_layout.hpp"

namespace alpha::render {

// The world-text atlas image last drawn per (font, pixel size), most recent
// first. Text is rasterized at the output size (window scale x camera zoom),
// and a font source publishes a new atlas image whenever it adds glyphs: the
// image it replaces, or the least recently used one beyond `capacity`, is
// told to `evicted` (to free its texture) once no entry shares it.
class FontCache {
 public:
  using Evicted = std::function<void(const core::Image*)>;
  FontCache(std::size_t capacity, Evicted evicted);

  void use(const core::FontSourcePtr& font, int pixels, const core::ImagePtr& image);
  std::size_t size() const { return entries_.size(); }

 private:
  struct Entry {
    core::FontSourcePtr font;  // held, so its address is never reused while cached
    int pixels = 0;
    core::ImagePtr image;      // held, so a texture's key is never reused while cached
  };
  void drop(const core::ImagePtr& image);
  std::size_t capacity_;
  Evicted evicted_;
  std::list<Entry> entries_;  // most recent first
};

}  // namespace alpha::render
