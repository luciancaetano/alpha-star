#include "platform/glyphs.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include "core/error.hpp"
#include "core/glyph_layout.hpp"

namespace alpha::platform {

namespace {

core::Error font_error(const std::filesystem::path& file, const std::string& why) {
  return core::Error("font \"" + file.generic_string() + "\": " + why);
}

// One coverage byte per pixel: 8-bit grey as is, 1-bit mono expanded to 0 / 255.
std::uint8_t coverage_at(const FT_Bitmap& bm, int x, int y) {
  const std::uint8_t* row = bm.buffer + static_cast<std::ptrdiff_t>(y) * bm.pitch;
  if (bm.pixel_mode != FT_PIXEL_MODE_MONO) return row[x];
  return (row[x / 8] & (0x80 >> (x % 8))) != 0 ? 255 : 0;
}

float to_pixels(FT_Pos v) { return std::round(static_cast<float>(v) / 64.0f); }

}  // namespace

struct FontFile::Impl {
  FT_Library library = nullptr;
  FT_Face face = nullptr;
  hb_blob_t* blob = nullptr;
  hb_face_t* hb_face = nullptr;
  int size = 0;  // pixel size the FreeType face is set to

  void set_size(int pixels) {
    const int p = std::clamp(pixels, 1, core::kMaxTextPixels);
    if (p == size) return;
    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(p));
    size = p;
  }
  ~Impl() {
    if (hb_face) hb_face_destroy(hb_face);
    if (blob) hb_blob_destroy(blob);
    if (face) FT_Done_Face(face);
    if (library) FT_Done_FreeType(library);
  }
};

FontFile::FontFile(vfs::Bytes bytes, std::filesystem::path file)
    : file_(std::move(file)), bytes_(std::move(bytes)), impl_(std::make_unique<Impl>()) {
  if (FT_Init_FreeType(&impl_->library) != 0) throw font_error(file_, "FreeType failed to start");
  if (FT_New_Memory_Face(impl_->library, bytes_.data(), static_cast<FT_Long>(bytes_.size()), 0, &impl_->face) != 0) {
    throw font_error(file_, "not a font FreeType can read (use .ttf, .otf or a BMFont .fnt)");
  }
  impl_->blob = hb_blob_create(reinterpret_cast<const char*>(bytes_.data()), static_cast<unsigned>(bytes_.size()),
                               HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  impl_->hb_face = hb_face_create(impl_->blob, 0);
}

FontFile::~FontFile() = default;

std::uint32_t FontFile::glyph_of(char32_t c) const {
  return static_cast<std::uint32_t>(FT_Get_Char_Index(impl_->face, static_cast<FT_ULong>(c)));
}

LineMetrics FontFile::metrics(int pixels) const {
  impl_->set_size(pixels);
  const FT_Size_Metrics& m = impl_->face->size->metrics;
  LineMetrics out{to_pixels(m.height), to_pixels(m.ascender), 0.0f};
  if (FT_Load_Char(impl_->face, 'x', FT_LOAD_DEFAULT) == 0) out.x_height = to_pixels(impl_->face->glyph->metrics.height);
  if (out.x_height <= 0.0f) out.x_height = std::round(out.ascent * 0.5f);
  return out;
}

std::vector<RunGlyph> FontFile::shape(const std::u32string& codes, std::size_t begin, std::size_t end, bool rtl,
                                      int pixels, std::uint32_t font) const {
  hb_buffer_t* buffer = hb_buffer_create();
  hb_buffer_add_utf32(buffer, reinterpret_cast<const std::uint32_t*>(codes.data()), static_cast<int>(codes.size()),
                      static_cast<unsigned>(begin), static_cast<int>(end - begin));
  hb_buffer_set_direction(buffer, rtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
  hb_buffer_guess_segment_properties(buffer);
  hb_font_t* hb = hb_font_create(impl_->hb_face);
  const int scale = std::clamp(pixels, 1, core::kMaxTextPixels) * 64;
  hb_font_set_scale(hb, scale, scale);
  hb_shape(hb, buffer, nullptr, 0);
  unsigned count = 0;
  const hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &count);
  const hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, &count);
  std::vector<RunGlyph> out;
  for (unsigned i = 0; i < count; ++i) {
    out.push_back({core::glyph_key(font, info[i].codepoint), static_cast<float>(pos[i].x_advance) / 64.0f,
                   static_cast<float>(pos[i].x_offset) / 64.0f, -static_cast<float>(pos[i].y_offset) / 64.0f,
                   info[i].cluster});
  }
  hb_font_destroy(hb);
  hb_buffer_destroy(buffer);
  return out;
}

// Hinted, so the bitmap and its offset are whole pixels.
GlyphBitmap FontFile::render(std::uint32_t id, int pixels, float ascent, bool crisp) const {
  impl_->set_size(pixels);
  const FT_Int32 flags = FT_LOAD_RENDER | (crisp ? FT_LOAD_TARGET_MONO : FT_LOAD_TARGET_NORMAL);
  if (FT_Load_Glyph(impl_->face, id, flags) != 0) return {};
  const FT_GlyphSlot slot = impl_->face->glyph;
  const FT_Bitmap& bm = slot->bitmap;
  GlyphBitmap out{static_cast<int>(bm.width), static_cast<int>(bm.rows), {}, {}, to_pixels(slot->advance.x)};
  out.coverage.reserve(static_cast<std::size_t>(out.width) * static_cast<std::size_t>(out.rows));
  for (int y = 0; y < out.rows; ++y) {
    for (int x = 0; x < out.width; ++x) out.coverage.push_back(coverage_at(bm, x, y));
  }
  out.offset = {static_cast<float>(slot->bitmap_left), ascent - static_cast<float>(slot->bitmap_top)};
  return out;
}

}  // namespace alpha::platform
