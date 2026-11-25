#include "platform/glyphs.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cmath>
#include <memory>
#include <type_traits>
#include <utility>
#include <string>
#include <vector>

#include "core/error.hpp"

namespace alpha::platform {

namespace {

constexpr int kAtlasWidth = 2048;  // kMaxTextPixels glyphs: 6 per row, about 32 rows
constexpr int kGap = 1;  // between glyphs: nearest filtering never bleeds a neighbor in

struct LibraryDeleter {
  void operator()(FT_Library lib) const { FT_Done_FreeType(lib); }
};
struct FaceDeleter {
  void operator()(FT_Face face) const { FT_Done_Face(face); }
};
using Library = std::unique_ptr<std::remove_pointer_t<FT_Library>, LibraryDeleter>;
using Face = std::unique_ptr<std::remove_pointer_t<FT_Face>, FaceDeleter>;

// One rendered glyph: coverage rows and where it goes.
struct Bitmap {
  int width = 0;
  int rows = 0;
  std::vector<std::uint8_t> coverage;
};

core::Error font_error(const std::filesystem::path& file, const std::string& why) {
  return core::Error("font \"" + file.generic_string() + "\": " + why);
}

// One coverage byte per pixel: 8-bit grey as is, 1-bit mono expanded to 0 / 255.
std::uint8_t coverage_at(const FT_Bitmap& bm, int x, int y) {
  const std::uint8_t* row = bm.buffer + static_cast<std::ptrdiff_t>(y) * bm.pitch;
  if (bm.pixel_mode != FT_PIXEL_MODE_MONO) return row[x];
  return (row[x / 8] & (0x80 >> (x % 8))) != 0 ? 255 : 0;
}

// Hinted, so the bitmap, its offset and the advance are whole pixels.
Bitmap render(FT_Face face, char32_t c, core::Glyph& glyph, float ascent, bool crisp) {
  const FT_Int32 flags = FT_LOAD_RENDER | (crisp ? FT_LOAD_TARGET_MONO : FT_LOAD_TARGET_NORMAL);
  if (FT_Load_Char(face, static_cast<FT_ULong>(c), flags) != 0) return {};
  const FT_GlyphSlot slot = face->glyph;
  const FT_Bitmap& bm = slot->bitmap;
  Bitmap out{static_cast<int>(bm.width), static_cast<int>(bm.rows), {}};
  for (int y = 0; y < out.rows; ++y) {
    for (int x = 0; x < out.width; ++x) out.coverage.push_back(coverage_at(bm, x, y));
  }
  glyph.offset = {static_cast<float>(slot->bitmap_left), ascent - static_cast<float>(slot->bitmap_top)};
  glyph.advance = std::round(static_cast<float>(slot->advance.x) / 64.0f);
  return out;
}

// Shelf packing, left to right then down; sets each glyph's src.
int pack(std::vector<Bitmap>& bitmaps, core::FontAtlas& atlas) {
  int x = kGap;
  int y = kGap;
  int shelf = 0;
  for (std::size_t i = 0; i < bitmaps.size(); ++i) {
    const Bitmap& b = bitmaps[i];
    if (x + b.width + kGap > kAtlasWidth) {
      x = kGap;
      y += shelf + kGap;
      shelf = 0;
    }
    atlas.glyphs[i].src = {x, y, b.width, b.rows};
    x += b.width + kGap;
    shelf = std::max(shelf, b.rows);
  }
  return y + shelf + kGap;
}

std::shared_ptr<core::Image> paint(const std::vector<Bitmap>& bitmaps, const core::FontAtlas& atlas, int height) {
  auto image = std::make_shared<core::Image>(core::Image{kAtlasWidth, height, {}, core::Filter::nearest});
  image->rgba.assign(static_cast<std::size_t>(kAtlasWidth) * static_cast<std::size_t>(height) * 4, 255);
  for (std::size_t p = 3; p < image->rgba.size(); p += 4) image->rgba[p] = 0;
  for (std::size_t i = 0; i < bitmaps.size(); ++i) {
    const core::Region& r = atlas.glyphs[i].src;
    for (int y = 0; y < r.h; ++y) {
      for (int x = 0; x < r.w; ++x) {
        const auto at = (static_cast<std::size_t>(r.y + y) * kAtlasWidth + static_cast<std::size_t>(r.x + x)) * 4;
        image->rgba[at + 3] = bitmaps[i].coverage[static_cast<std::size_t>(y * r.w + x)];
      }
    }
  }
  return image;
}

}  // namespace

core::FontAtlasPtr rasterize_font(const vfs::Bytes& bytes, const std::filesystem::path& file, int pixels, bool crisp) {
  FT_Library raw_lib = nullptr;
  if (FT_Init_FreeType(&raw_lib) != 0) throw font_error(file, "FreeType failed to start");
  const Library lib(raw_lib);
  FT_Face raw_face = nullptr;
  if (FT_New_Memory_Face(lib.get(), bytes.data(), static_cast<FT_Long>(bytes.size()), 0, &raw_face) != 0) {
    throw font_error(file, "not a font FreeType can read (use .ttf, .otf or a BMFont .fnt)");
  }
  const Face face(raw_face);
  FT_Set_Pixel_Sizes(face.get(), 0, static_cast<FT_UInt>(std::clamp(pixels, 1, core::kMaxTextPixels)));
  auto atlas = std::make_shared<core::FontAtlas>();
  atlas->ascent = std::round(static_cast<float>(face->size->metrics.ascender) / 64.0f);
  atlas->line_height = std::round(static_cast<float>(face->size->metrics.height) / 64.0f);
  std::vector<Bitmap> bitmaps;
  for (std::size_t i = 0; i < core::kGlyphCount; ++i) {
    bitmaps.push_back(render(face.get(), core::glyph_code(i), atlas->glyphs[i], atlas->ascent, crisp));
  }
  auto image = paint(bitmaps, *atlas, pack(bitmaps, *atlas));
  image->filter = crisp ? core::Filter::nearest : core::Filter::linear;
  atlas->image = std::move(image);
  return atlas;
}

}  // namespace alpha::platform
