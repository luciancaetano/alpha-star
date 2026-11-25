#include "render/font_cache.hpp"

#include <algorithm>
#include <utility>

namespace alpha::render {

FontCache::FontCache(std::size_t capacity, Evicted evicted) : capacity_(capacity), evicted_(std::move(evicted)) {}

core::FontAtlasPtr FontCache::get(const core::FontSourcePtr& font, int pixels) {
  const auto hit = std::find_if(entries_.begin(), entries_.end(),
                                [&](const Entry& e) { return e.font == font && e.pixels == pixels; });
  if (hit != entries_.end()) {
    entries_.splice(entries_.begin(), entries_, hit);
    return entries_.front().atlas;
  }
  entries_.push_front({font, pixels, font->atlas(pixels)});
  if (entries_.size() > capacity_) {
    const core::Image* image = entries_.back().atlas->image.get();
    entries_.pop_back();
    const bool shared = std::any_of(entries_.begin(), entries_.end(),
                                    [&](const Entry& e) { return e.atlas->image.get() == image; });
    if (!shared) evicted_(image);
  }
  return entries_.front().atlas;
}

}  // namespace alpha::render
