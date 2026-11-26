#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "platform/glyph_atlas.hpp"
#include "platform/text_shaper.hpp"
#include "platform/vfs.hpp"

// One TrueType / OpenType font in memory: HarfBuzz shapes runs of it
// (ligatures, kerning, Arabic joining, Indic reordering), FreeType rasterizes
// its glyphs hinted at whole pixels. Positions come out in atlas pixels.
namespace alpha::platform {

class FontFile {
 public:
  // Throws core::Error naming `file` when it cannot be parsed.
  FontFile(vfs::Bytes bytes, std::filesystem::path file);
  ~FontFile();
  FontFile(const FontFile&) = delete;
  FontFile& operator=(const FontFile&) = delete;

  const std::filesystem::path& file() const { return file_; }
  // Glyph id of code point c, 0 when the font lacks it.
  std::uint32_t glyph_of(char32_t c) const;
  LineMetrics metrics(int pixels) const;
  // Glyphs of codes [begin, end) in one direction and script, visual order;
  // keys use `font` as the chain index, clusters index `codes`.
  std::vector<RunGlyph> shape(const std::u32string& codes, std::size_t begin, std::size_t end, bool rtl, int pixels,
                              std::uint32_t font) const;
  // Glyph `id` at `pixels` (crisp = monochrome); y offsets from a line top `ascent` above the baseline.
  GlyphBitmap render(std::uint32_t id, int pixels, float ascent, bool crisp) const;

 private:
  struct Impl;
  std::filesystem::path file_;
  vfs::Bytes bytes_;  // FreeType and HarfBuzz read the font from here
  std::unique_ptr<Impl> impl_;
};

}  // namespace alpha::platform
