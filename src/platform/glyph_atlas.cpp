#include "platform/glyph_atlas.hpp"

#include <algorithm>
#include <cstring>
#include <memory>

#include "core/limits.hpp"

namespace alpha::platform {

namespace {

constexpr int kSide = core::kMaxGlyphAtlasSide;
constexpr int kGap = 1;          // between glyphs: filtering never bleeds a neighbor in
constexpr int kGrowRows = 256;   // the atlas grows this many rows at a time

}  // namespace

GlyphAtlas::GlyphAtlas(LineMetrics metrics, core::Filter filter) : filter_(filter) {
  glyphs_.line_height = metrics.line_height;
  glyphs_.ascent = metrics.ascent;
  glyphs_.x_height = metrics.x_height;
  restart();
}

void GlyphAtlas::restart() {
  glyphs_.glyphs.clear();
  coverage_.clear();
  height_ = 0;
  x_ = kGap;
  y_ = kGap;
  shelf_ = 0;
}

// Shelf packing, left to right then down; false when the atlas is full.
bool GlyphAtlas::place(core::GlyphKey key, const GlyphBitmap& b) {
  if (b.width + 2 * kGap > kSide || b.rows + 2 * kGap > kSide) return false;
  if (x_ + b.width + kGap > kSide) {
    x_ = kGap;
    y_ += shelf_ + kGap;
    shelf_ = 0;
  }
  const int bottom = y_ + b.rows + kGap;
  if (bottom > kSide) return false;
  if (bottom > height_) {
    height_ = std::min(kSide, (bottom + kGrowRows - 1) / kGrowRows * kGrowRows);
    coverage_.resize(static_cast<std::size_t>(kSide) * static_cast<std::size_t>(height_), 0);
  }
  for (int y = 0; y < b.rows; ++y) {
    std::memcpy(coverage_.data() + static_cast<std::size_t>(y_ + y) * kSide + static_cast<std::size_t>(x_),
                b.coverage.data() + static_cast<std::size_t>(y * b.width), static_cast<std::size_t>(b.width));
  }
  glyphs_.glyphs[key] = {{x_, y_, b.width, b.rows}, b.offset, b.advance};
  x_ += b.width + kGap;
  shelf_ = std::max(shelf_, b.rows);
  return true;
}

void GlyphAtlas::publish() {
  auto image = std::make_shared<core::Image>(core::Image{kSide, std::max(1, height_), {}, filter_});
  image->rgba.assign(static_cast<std::size_t>(image->width) * static_cast<std::size_t>(image->height) * 4, 255);
  for (std::size_t p = 0; p < coverage_.size(); ++p) image->rgba[p * 4 + 3] = coverage_[p];
  if (coverage_.empty()) image->rgba[3] = 0;
  auto atlas = std::make_shared<core::FontAtlas>(glyphs_);
  atlas->image = std::move(image);
  published_ = std::move(atlas);
}

bool GlyphAtlas::refill(const std::vector<core::GlyphKey>& keys, const Made& made, const Render& render) {
  restart();
  bool all = true;
  for (const core::GlyphKey k : keys) {
    const auto it = std::find_if(made.begin(), made.end(), [k](const auto& m) { return m.first == k; });
    all = place(k, it != made.end() ? it->second : render(k)) && all;
  }
  return all;
}

core::FontAtlasPtr GlyphAtlas::ensure(const std::vector<core::GlyphKey>& keys, const Render& render, bool& dropped) {
  std::vector<core::GlyphKey> missing;
  for (const core::GlyphKey k : keys) {
    if (!glyphs_.glyphs.contains(k)) missing.push_back(k);
  }
  dropped = false;
  if (missing.empty() && published_) return published_;
  Made made;
  for (const core::GlyphKey k : missing) made.emplace_back(k, render(k));
  bool fits = true;
  for (const auto& [k, b] : made) fits = fits && place(k, b);
  if (!fits) dropped = !refill(keys, made, render);  // start over with only what this text needs
  publish();
  return published_;
}

}  // namespace alpha::platform
