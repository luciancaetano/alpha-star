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
#include "platform/glyphs.hpp"
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

class TrueType final : public core::FontSource {
 public:
  TrueType(std::filesystem::path file, bool crisp) : file_(std::move(file)), bytes_(read_file(file_)), crisp_(crisp) {
    atlas(8);  // fails now, naming the file, if FreeType cannot read it
  }
  int native() const override { return 0; }
  core::FontAtlasPtr atlas(int pixels) const override { return rasterize_font(bytes_, file_, pixels, crisp_); }

 private:
  std::filesystem::path file_;
  vfs::Bytes bytes_;
  bool crisp_;
};

// One atlas at the native size, made at load.
class Bitmap final : public core::FontSource {
 public:
  Bitmap(core::FontAtlasPtr atlas, int native) : atlas_(std::move(atlas)), native_(native) {}
  int native() const override { return native_; }
  core::FontAtlasPtr atlas(int) const override { return atlas_; }

 private:
  core::FontAtlasPtr atlas_;
  int native_;
};

// The pages stacked top to bottom in one image; page_top gets each page's first row.
std::shared_ptr<core::Image> stack(const std::vector<core::Image>& pages, std::vector<int>& page_top) {
  int width = 0;
  int height = 0;
  for (const core::Image& p : pages) {
    page_top.push_back(height);
    width = std::max(width, p.width);
    height += p.height;
  }
  auto image = std::make_shared<core::Image>(core::Image{width, height, {}, false});
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
  image->smooth = !pixel_mode;
  auto atlas = std::make_shared<core::FontAtlas>(core::bitmap_atlas(font, page_top));
  atlas->image = std::move(image);
  return std::make_shared<Bitmap>(std::move(atlas), font.size);
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

core::FontSourcePtr make_default() {
  constexpr int kPitch = kCell + 1;
  const int rows = (kDefaultFontCells + kColumns - 1) / kColumns;
  core::Image image{kColumns * kPitch + 1, rows * kPitch + 1, {}, false};
  image.rgba.assign(static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4, 255);
  for (std::size_t p = 3; p < image.rgba.size(); p += 4) image.rgba[p] = 0;
  auto atlas = std::make_shared<core::FontAtlas>();
  atlas->line_height = kCell;
  atlas->ascent = kCell - 1;
  for (std::size_t g = 0; g < core::kGlyphCount; ++g) {
    const char32_t c = core::glyph_code(g);
    atlas->glyphs[g].advance = kCell;
    if (c == U' ' || c == 0xA0) continue;
    const int i = c <= U'~' ? static_cast<int>(c) - 0x21 : static_cast<int>(c) - 0xA1 + (0x7F - 0x21);
    const core::Region at{1 + (i % kColumns) * kPitch, 1 + (i / kColumns) * kPitch, kCell, kCell};
    paint_cell(image, i, at);
    atlas->glyphs[g].src = at;
  }
  atlas->image = std::make_shared<core::Image>(std::move(image));
  return std::make_shared<Bitmap>(std::move(atlas), kCell);
}

}  // namespace

bool is_bitmap_font(const std::filesystem::path& file) { return vfs::lower_extension(file) == ".fnt"; }

core::FontSourcePtr load_font(const std::filesystem::path& file, bool pixel_mode) {
  if (is_bitmap_font(file)) return load_bitmap(file, pixel_mode);
  return std::make_shared<TrueType>(file, pixel_mode);
}

core::FontSourcePtr default_font() {
  static const core::FontSourcePtr font = make_default();
  return font;
}

}  // namespace alpha::platform
