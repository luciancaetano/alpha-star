#include "platform/fonts.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/bmfont.hpp"
#include "core/error.hpp"
#include "platform/assets.hpp"
#include "platform/default_font_data.hpp"
#include "core/utf8.hpp"
#include "platform/run_items.hpp"
#include "platform/text_shaper.hpp"
#include "platform/truetype.hpp"
#include "platform/vfs.hpp"

namespace alpha::platform {

namespace {

constexpr int kCell = 8;      // built-in font: 8x8 glyphs
constexpr int kColumns = 16;  // per atlas row, 1 px gap around each

core::Error font_error(const std::filesystem::path& file, const std::string& why) {
  return core::Error("font \"" + file.generic_string() + "\": " + why);
}

vfs::Bytes read_file(const std::filesystem::path& file) {
  auto bytes = vfs::read(file);
  if (!bytes) throw font_error(file, "cannot read the file (paths are relative to the game's main file)");
  return std::move(*bytes);
}

// Bitmap fonts shape one glyph per code point (kerning pairs apply); what
// the font lacks draws as its box glyph.
class BitmapShaper final : public RunShaper {
 public:
  BitmapShaper(const core::BitmapAtlas& font, MissingGlyphs& missing) : font_(font), missing_(missing) {}
  std::vector<RunGlyph> shape_run(const std::u32string& codes, std::size_t begin, std::size_t end, bool rtl,
                                  int) override {
    std::vector<RunGlyph> out;
    for (std::size_t i = begin; i < end; ++i) {
      if (is_invisible(codes[i])) continue;
      if (!out.empty()) out.back().advance += kerning(codes[i - 1], codes[i]);
      out.push_back({key_of(codes[i]), 0.0f, 0.0f, 0.0f, i});
      out.back().advance = font_.atlas.glyphs.at(out.back().key).advance;
    }
    if (rtl) std::reverse(out.begin(), out.end());
    return out;
  }

 private:
  core::GlyphKey key_of(char32_t c) {
    const core::GlyphKey key = core::glyph_key(0, static_cast<std::uint32_t>(c));
    if (font_.atlas.glyphs.contains(key)) return key;
    missing_.report(c);
    return core::kTofuKey;
  }
  float kerning(char32_t a, char32_t b) const {
    const auto it = font_.kerning.find({a, b});
    return it == font_.kerning.end() ? 0.0f : it->second;
  }
  const core::BitmapAtlas& font_;
  MissingGlyphs& missing_;
};

// One atlas at the native size, made at load.
class Bitmap final : public core::FontSource {
 public:
  Bitmap(core::BitmapAtlas font, int native, const std::string& name)
      : font_(std::move(font)), atlas_(std::make_shared<core::FontAtlas>(font_.atlas)), native_(native), missing_(name) {}
  int native() const override { return native_; }
  core::ShapedText shape(const core::TextRequest& r) const override {
    BitmapShaper shaper(font_, missing_);
    return {atlas_, shape_lines(shaper, core::decode_utf8(r.text), native_, r.dir, r.wrap)};
  }

 private:
  core::BitmapAtlas font_;
  core::FontAtlasPtr atlas_;
  int native_;
  mutable MissingGlyphs missing_;
};

// A box glyph (kTofuKey) in new rows under the image: an outline half an em wide.
void add_tofu(core::Image& image, core::FontAtlas& atlas, int native) {
  const int w = std::max(3, native * 45 / 100);
  const int h = std::max(4, static_cast<int>(atlas.ascent * 0.8f));
  const int top = image.height + 1;
  const int width = std::max(image.width, w + 2);
  core::Image grown{width, top + h + 1, {}, image.filter};
  grown.rgba.assign(static_cast<std::size_t>(grown.width) * static_cast<std::size_t>(grown.height) * 4, 0);
  for (int y = 0; y < image.height; ++y) {
    std::memcpy(grown.rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4,
                image.rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) * 4,
                static_cast<std::size_t>(image.width) * 4);
  }
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (x != 0 && y != 0 && x != w - 1 && y != h - 1) continue;
      const auto p = (static_cast<std::size_t>(top + y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(1 + x)) * 4;
      for (std::size_t c = 0; c < 4; ++c) grown.rgba[p + c] = 255;
    }
  }
  atlas.glyphs[core::kTofuKey] = {{1, top, w, h}, {1, atlas.ascent - static_cast<float>(h)}, static_cast<float>(w + 2)};
  image = std::move(grown);
}

// The pages stacked top to bottom in one image; page_top gets each page's first row.
std::shared_ptr<core::Image> stack(const std::vector<core::Image>& pages, std::vector<int>& page_top) {
  int width = 0;
  int height = 0;
  for (const core::Image& p : pages) {
    page_top.push_back(height);
    width = std::max(width, p.width);
    height += p.height;
  }
  auto image = std::make_shared<core::Image>(core::Image{width, height, {}, core::Filter::nearest});
  image->rgba.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0);
  for (std::size_t i = 0; i < pages.size(); ++i) {
    const auto row = static_cast<std::size_t>(pages[i].width) * 4;
    for (int y = 0; y < pages[i].height; ++y) {
      const auto at = (static_cast<std::size_t>(page_top[i] + y) * static_cast<std::size_t>(width)) * 4;
      std::memcpy(image->rgba.data() + at, pages[i].rgba.data() + row * static_cast<std::size_t>(y), row);
    }
  }
  return image;
}

core::FontSourcePtr load_bitmap(const std::filesystem::path& file, bool pixel_mode) {
  const vfs::Bytes bytes = read_file(file);
  core::BmFont font;
  try {
    font = core::parse_bmfont({reinterpret_cast<const char*>(bytes.data()), bytes.size()});
  } catch (const core::Error& e) {
    throw font_error(file, e.what());
  }
  std::vector<core::Image> pages;
  for (const std::string& page : font.pages) pages.push_back(load_image((file.parent_path() / page).string()));
  std::vector<int> page_top;
  auto image = stack(pages, page_top);
  image->filter = pixel_mode ? core::Filter::nearest : core::Filter::linear;
  core::BitmapAtlas atlas = core::bitmap_atlas(font, page_top);
  add_tofu(*image, atlas.atlas, font.size);
  atlas.atlas.image = std::move(image);
  return std::make_shared<Bitmap>(std::move(atlas), font.size, "font \"" + file.generic_string() + "\"");
}

// Built-in glyph i (0 = U+0021) into the atlas at its cell.
void paint_cell(core::Image& image, int i, const core::Region& at) {
  for (int y = 0; y < kCell; ++y) {
    const std::uint8_t bits = kDefaultFontBits[static_cast<std::size_t>(i * kCell + y)];
    for (int x = 0; x < kCell; ++x) {
      const auto p = (static_cast<std::size_t>(at.y + y) * static_cast<std::size_t>(image.width) +
                      static_cast<std::size_t>(at.x + x)) * 4;
      image.rgba[p + 3] = (bits & (1 << x)) != 0 ? 255 : 0;
    }
  }
}

// Cell of code point c in the built-in font data (ASCII from '!', then Latin-1 from U+00A1).
int default_cell(char32_t c) { return c <= U'~' ? static_cast<int>(c) - 0x21 : static_cast<int>(c) - 0xA1 + (0x7F - 0x21); }

core::FontSourcePtr make_default() {
  constexpr int kPitch = kCell + 1;
  const int rows = (kDefaultFontCells + kColumns - 1) / kColumns;
  core::Image image{kColumns * kPitch + 1, rows * kPitch + 1, {}, core::Filter::nearest};
  image.rgba.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4, 255);
  for (std::size_t p = 3; p < image.rgba.size(); p += 4) image.rgba[p] = 0;
  core::BitmapAtlas font;
  font.atlas.line_height = kCell;
  font.atlas.ascent = kCell - 1;
  font.atlas.x_height = kCell / 2;
  for (char32_t c = U' '; c <= 0xFF; ++c) {
    if (c > U'~' && c < 0xA0) continue;
    core::Glyph& g = font.atlas.glyphs[core::glyph_key(0, static_cast<std::uint32_t>(c))];
    g.advance = kCell;
    if (c == U' ' || c == 0xA0) continue;
    const int i = default_cell(c);
    g.src = {1 + (i % kColumns) * kPitch, 1 + (i / kColumns) * kPitch, kCell, kCell};
    paint_cell(image, i, g.src);
  }
  add_tofu(image, font.atlas, kCell);
  font.atlas.image = std::make_shared<core::Image>(std::move(image));
  return std::make_shared<Bitmap>(std::move(font), kCell, "the built-in font");
}

}  // namespace

bool is_bitmap_font(const std::filesystem::path& file) { return vfs::lower_extension(file) == ".fnt"; }

core::FontSourcePtr load_font(const std::filesystem::path& file, bool pixel_mode,
                              const std::vector<core::FontSourcePtr>& fallbacks) {
  if (!is_bitmap_font(file)) return load_truetype(file, pixel_mode, fallbacks);
  if (!fallbacks.empty()) {
    throw font_error(file, "fallbacks need a .ttf/.otf font; a BMFont draws a box for what it lacks");
  }
  return load_bitmap(file, pixel_mode);
}

core::FontSourcePtr default_font() {
  static const core::FontSourcePtr font = make_default();
  return font;
}

}  // namespace alpha::platform
