#include "render/font_cache.hpp"

#include <algorithm>
#include <utility>

namespace alpha::render {

FontCache::FontCache(std::size_t capacity, Evicted evicted) : capacity_(capacity), evicted_(std::move(evicted)) {}

void FontCache::drop(const core::ImagePtr& image) {
  const bool shared = std::any_of(entries_.begin(), entries_.end(), [&](const Entry& e) { return e.image == image; });
  if (!shared) evicted_(image.get());
}

void FontCache::use(const core::FontSourcePtr& font, int pixels, const core::ImagePtr& image) {
  const auto hit = std::find_if(entries_.begin(), entries_.end(),
                                [&](const Entry& e) { return e.font == font && e.pixels == pixels; });
  if (hit != entries_.end()) {
    entries_.splice(entries_.begin(), entries_, hit);
    if (entries_.front().image == image) return;
    const core::ImagePtr old = std::exchange(entries_.front().image, image);
    drop(old);
    return;
  }
  entries_.push_front({font, pixels, image});
  if (entries_.size() <= capacity_) return;
  const core::ImagePtr old = entries_.back().image;
  entries_.pop_back();
  drop(old);
}

}  // namespace alpha::render
