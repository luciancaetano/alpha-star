#pragma once

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/glyph_layout.hpp"

// A font's glyphs at one pixel size, rasterized on demand into one growing
// RGBA atlas (white, alpha = coverage). Up to core::kMaxGlyphAtlasSide square;
// when the glyphs a text needs do not fit, it starts over with just those, so
// any number of distinct glyphs (CJK) works with bounded memory. Each change
// publishes a new atlas image (the renderer swaps textures by image).
namespace alpha::platform {

struct GlyphBitmap {
  int width = 0;
  int rows = 0;
  std::vector<std::uint8_t> coverage;  // width x rows
  core::Vec2 offset;                   // top-left from the pen, y from the line top
  float advance = 0.0f;
};

struct LineMetrics {
  float line_height = 0.0f;
  float ascent = 0.0f;
  float x_height = 0.0f;
};

class GlyphAtlas {
 public:
  using Render = std::function<GlyphBitmap(core::GlyphKey)>;
  GlyphAtlas(LineMetrics metrics, core::Filter filter);

  // The atlas holding `keys` (as many as fit in a fresh atlas; the rest are
  // skipped, reported through `dropped`).
  core::FontAtlasPtr ensure(const std::vector<core::GlyphKey>& keys, const Render& render, bool& dropped);

 private:
  using Made = std::vector<std::pair<core::GlyphKey, GlyphBitmap>>;
  bool place(core::GlyphKey key, const GlyphBitmap& bitmap);
  bool refill(const std::vector<core::GlyphKey>& keys, const Made& made, const Render& render);
  void restart();
  void publish();

  core::FontAtlas glyphs_;  // the working copy (image unset)
  core::Filter filter_;
  std::vector<std::uint8_t> coverage_;  // kMaxGlyphAtlasSide wide, height_ rows
  int height_ = 0;
  int x_ = 0;
  int y_ = 0;
  int shelf_ = 0;
  core::FontAtlasPtr published_;
};

}  // namespace alpha::platform
