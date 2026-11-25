#include "platform/glyphs.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <memory>
#include <type_traits>
#include <string>
#include <vector>

#include "core/error.hpp"
#include "platform/vfs.hpp"

namespace alpha::platform {

namespace {

constexpr int kAtlasWidth = 512;
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

vfs::Bytes read_file(const std::filesystem::path& file) {
  auto bytes = vfs::read(file);
  if (!bytes) throw font_error(file, "cannot read the file (paths are relative to the game's main file)");
  return std::move(*bytes);
}

Bitmap render(FT_Face face, char32_t c, core::Glyph& glyph, float ascent) {
  if (FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_RENDER) != 0) return {};
  const FT_GlyphSlot slot = face->glyph;
  const FT_Bitmap& bm = slot->bitmap;
  Bitmap out{static_cast<int>(bm.width), static_cast<int>(bm.rows), {}};
  for (int y = 0; y < out.rows; ++y) {
    const std::uint8_t* row = bm.buffer + static_cast<std::ptrdiff_t>(y) * bm.pitch;
    out.coverage.insert(out.coverage.end(), row, row + out.width);
  }
  glyph.offset = {static_cast<float>(slot->bitmap_left), ascent - static_cast<float>(slot->bitmap_top)};
  glyph.advance = static_cast<float>(slot->advance.x) / 64.0f;
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

core::ImagePtr paint(const std::vector<Bitmap>& bitmaps, const core::FontAtlas& atlas, int height) {
  auto image = std::make_shared<core::Image>(core::Image{kAtlasWidth, height, {}, false});
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

core::FontAtlasPtr rasterize_font(const std::filesystem::path& file, int pixels) {
  const vfs::Bytes bytes = read_file(file);
  FT_Library raw_lib = nullptr;
  if (FT_Init_FreeType(&raw_lib) != 0) throw font_error(file, "FreeType failed to start");
  const Library lib(raw_lib);
  FT_Face raw_face = nullptr;
  if (FT_New_Memory_Face(lib.get(), bytes.data(), static_cast<FT_Long>(bytes.size()), 0, &raw_face) != 0) {
    throw font_error(file, "not a font FreeType can read (use .ttf or .otf)");
  }
  const Face face(raw_face);
  FT_Set_Pixel_Sizes(face.get(), 0, static_cast<FT_UInt>(std::clamp(pixels, 1, 256)));
  auto atlas = std::make_shared<core::FontAtlas>();
  const float ascent = static_cast<float>(face->size->metrics.ascender) / 64.0f;
  atlas->line_height = static_cast<float>(face->size->metrics.height) / 64.0f;
  std::vector<Bitmap> bitmaps;
  for (std::size_t i = 0; i < core::kGlyphCount; ++i) {
    bitmaps.push_back(render(face.get(), core::glyph_code(i), atlas->glyphs[i], ascent));
  }
  atlas->image = paint(bitmaps, *atlas, pack(bitmaps, *atlas));
  return atlas;
}

}  // namespace alpha::platform
