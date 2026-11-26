#pragma once

#include <cstddef>
#include <functional>
#include <set>
#include <string>
#include <vector>

#include "core/glyph_layout.hpp"

// The one text pipeline every font source goes through: paragraphs at '\n',
// bidi (platform/bidi), wrapping (core/line_break), then each line's runs
// shaped by the source (HarfBuzz for TrueType, a code point lookup for bitmap
// fonts) and placed on whole atlas pixels.
namespace alpha::platform {

// A shaped glyph: advance and offsets in atlas pixels (y down); cluster = the
// index of the first code point it draws.
struct RunGlyph {
  core::GlyphKey key = 0;
  float advance = 0.0f;
  float x_offset = 0.0f;
  float y_offset = 0.0f;
  std::size_t cluster = 0;
};

class RunShaper {
 public:
  virtual ~RunShaper() = default;
  // Glyphs of codes [begin, end), all in one direction, in visual order (left to right).
  virtual std::vector<RunGlyph> shape_run(const std::u32string& codes, std::size_t begin, std::size_t end, bool rtl,
                                          int pixels) = 0;
};

// Lines of `codes` in visual order, wrapped at `wrap` atlas pixels (0 = only '\n').
std::vector<core::ShapedLine> shape_lines(RunShaper& shaper, const std::u32string& codes, int pixels, core::TextDir dir,
                                          float wrap);

// Every glyph key the lines use (each once).
std::vector<core::GlyphKey> keys_of(const std::vector<core::ShapedLine>& lines);

// Where text warnings go (SDL_Log by default; tests capture them).
using TextWarnings = std::function<void(const std::string&)>;
void set_text_warnings(TextWarnings sink);

// Reports code points no font has, once per font and Unicode block (128 code points).
class MissingGlyphs {
 public:
  explicit MissingGlyphs(std::string font) : font_(std::move(font)) {}
  void report(char32_t c);
  // Once per pixel size: a single text needs more glyphs than one atlas holds.
  void report_full(int pixels);

 private:
  std::string font_;
  std::set<char32_t> blocks_;
  std::set<int> full_;
};

}  // namespace alpha::platform
