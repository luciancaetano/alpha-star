#include "platform/run_items.hpp"

#include <hb.h>

namespace alpha::platform {

namespace {

hb_unicode_funcs_t* unicode() { return hb_unicode_funcs_get_default(); }

bool is_mark(char32_t c) {
  const hb_unicode_general_category_t g = hb_unicode_general_category(unicode(), c);
  return g == HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK || g == HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK ||
         g == HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK || g == HB_UNICODE_GENERAL_CATEGORY_FORMAT ||
         (c >= 0xFE00 && c <= 0xFE0F) || (c >= 0xE0100 && c <= 0xE01EF);
}

bool is_neutral(hb_script_t s) {
  return s == HB_SCRIPT_COMMON || s == HB_SCRIPT_INHERITED || s == HB_SCRIPT_UNKNOWN;
}

std::uint32_t first_font(char32_t c, std::uint32_t fonts, const Covers& covers) {
  for (std::uint32_t f = 0; f < fonts; ++f) {
    if (covers(f, c)) return f;
  }
  return kNoFont;
}

// Whether c (script s) joins the item drawn by `font` whose script is `script`.
bool joins(char32_t c, hb_script_t s, std::uint32_t font, hb_script_t script, std::uint32_t fonts, const Covers& covers) {
  if (is_mark(c)) return true;
  if (font == kNoFont) return first_font(c, fonts, covers) == kNoFont;
  const bool same_script = is_neutral(s) || is_neutral(script) || s == script;
  return same_script && covers(font, c);
}

}  // namespace

bool is_invisible(char32_t c) {
  if (c < 0x20 || c == 0x7F) return true;
  return hb_unicode_general_category(unicode(), c) == HB_UNICODE_GENERAL_CATEGORY_FORMAT;
}

std::vector<RunItem> run_items(const std::u32string& codes, std::size_t begin, std::size_t end, std::uint32_t fonts,
                               const Covers& covers) {
  std::vector<RunItem> out;
  hb_script_t script = HB_SCRIPT_COMMON;  // of the last item, once it has a strong letter
  for (std::size_t i = begin; i < end; ++i) {
    const char32_t c = codes[i];
    const hb_script_t s = hb_unicode_script(unicode(), c);
    if (!out.empty() && joins(c, s, out.back().font, script, fonts, covers)) {
      out.back().end = i + 1;
      if (!is_neutral(s)) script = s;
      continue;
    }
    out.push_back({i, i + 1, first_font(c, fonts, covers)});
    script = s;
  }
  return out;
}

}  // namespace alpha::platform
