#include "platform/text_shaper.hpp"

#include <SDL3/SDL.h>

#include <cmath>
#include <cstdio>
#include <memory>
#include <unordered_set>
#include <utility>

#include "core/line_break.hpp"
#include "platform/bidi.hpp"

namespace alpha::platform {

namespace {

TextWarnings& warnings() {
  static TextWarnings sink = [](const std::string& m) { SDL_Log("[text] %s", m.c_str()); };
  return sink;
}

struct Paragraph {
  std::size_t begin = 0;
  std::size_t end = 0;
  std::unique_ptr<BidiParagraph> bidi;
};

std::vector<Paragraph> paragraphs(const std::u32string& codes, core::TextDir dir) {
  std::vector<Paragraph> out;
  std::size_t begin = 0;
  for (std::size_t i = 0; i <= codes.size(); ++i) {
    if (i < codes.size() && codes[i] != U'\n') continue;
    out.push_back({begin, i, std::make_unique<BidiParagraph>(codes, begin, i, dir)});
    begin = i + 1;
  }
  return out;
}

// Width of each code point in logical order (a ligature counts on its first code point).
std::vector<float> advances(RunShaper& shaper, const std::u32string& codes, const std::vector<Paragraph>& pars, int pixels) {
  std::vector<float> out(codes.size(), 0.0f);
  for (const Paragraph& p : pars) {
    for (const BidiRun& run : p.bidi->logical_runs()) {
      for (const RunGlyph& g : shaper.shape_run(codes, run.begin, run.end, run.rtl, pixels)) out[g.cluster] += g.advance;
    }
  }
  return out;
}

const Paragraph& paragraph_of(const std::vector<Paragraph>& pars, std::size_t at) {
  for (const Paragraph& p : pars) {
    if (at >= p.begin && at <= p.end) return p;
  }
  return pars.back();
}

core::ShapedLine place(RunShaper& shaper, const std::u32string& codes, const Paragraph& par, core::LineRange range,
                       int pixels) {
  core::ShapedLine line;
  float pen = 0.0f;
  for (const BidiRun& run : par.bidi->line_runs(range.begin, range.end)) {
    for (const RunGlyph& g : shaper.shape_run(codes, run.begin, run.end, run.rtl, pixels)) {
      line.glyphs.push_back({g.key, std::round(pen + g.x_offset), std::round(g.y_offset)});
      pen += g.advance;
    }
  }
  line.width = std::round(pen);
  return line;
}

}  // namespace

std::vector<core::ShapedLine> shape_lines(RunShaper& shaper, const std::u32string& codes, int pixels, core::TextDir dir,
                                          float wrap) {
  const std::vector<Paragraph> pars = paragraphs(codes, dir);
  const std::vector<float> widths = advances(shaper, codes, pars, pixels);
  std::vector<core::ShapedLine> out;
  for (const core::LineRange& range : core::wrap_lines(codes, widths, wrap)) {
    out.push_back(place(shaper, codes, paragraph_of(pars, range.begin), range, pixels));
  }
  return out;
}

std::vector<core::GlyphKey> keys_of(const std::vector<core::ShapedLine>& lines) {
  std::vector<core::GlyphKey> out;
  std::unordered_set<core::GlyphKey> seen;
  for (const core::ShapedLine& l : lines) {
    for (const core::PlacedGlyph& g : l.glyphs) {
      if (seen.insert(g.key).second) out.push_back(g.key);
    }
  }
  return out;
}

void set_text_warnings(TextWarnings sink) {
  warnings() = sink ? std::move(sink) : [](const std::string& m) { SDL_Log("[text] %s", m.c_str()); };
}

void MissingGlyphs::report(char32_t c) {
  const char32_t block = c & ~char32_t{0x7F};
  if (!blocks_.insert(block).second) return;
  char range[64];
  std::snprintf(range, sizeof range, "U+%04X (block U+%04X..U+%04X)", static_cast<unsigned>(c), static_cast<unsigned>(block),
                static_cast<unsigned>(block + 0x7F));
  warnings()(font_ + " has no glyph for " + range +
             ": drawn as a box. Add a font that covers it to the font's fallbacks (assets.fonts.<name>.fallbacks).");
}

void MissingGlyphs::report_full(int pixels) {
  if (!full_.insert(pixels).second) return;
  warnings()(font_ + " at " + std::to_string(pixels) +
             " px: one text needs more glyphs than an atlas holds; some are skipped (draw it smaller or split it)");
}

}  // namespace alpha::platform
