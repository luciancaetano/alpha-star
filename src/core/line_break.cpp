#include "core/line_break.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace alpha::core {

namespace {

bool is_space(char32_t c) { return c == U' ' || c == U'\t' || c == 0x3000; }

// Not at the start of a line: closing marks, iteration marks, small kana.
bool closes(char32_t c) {
  static constexpr std::u32string_view kClosing =
      U"、。，．！？：；）」』】〉》〕｝〗〙〛ー々ゝゞヽヾぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮヵヶ…‥・])}.,!?:;%";
  return kClosing.find(c) != std::u32string_view::npos;
}

// Not at the end of a line: opening brackets and quotes.
bool opens(char32_t c) {
  static constexpr std::u32string_view kOpening = U"（「『【〈《〔｛〖〘〚([{";
  return kOpening.find(c) != std::u32string_view::npos;
}

std::size_t trim_end(const std::u32string& codes, std::size_t begin, std::size_t end) {
  while (end > begin && is_space(codes[end - 1])) --end;
  return end;
}

std::size_t skip_spaces(const std::u32string& codes, std::size_t i, std::size_t end) {
  while (i < end && is_space(codes[i])) ++i;
  return i;
}

// Breaks one paragraph [begin, end) (no '\n' inside) into lines.
void wrap_paragraph(const std::u32string& codes, const std::vector<float>& advances, float width, LineRange par,
                    std::vector<LineRange>& out) {
  std::size_t start = par.begin;
  std::size_t last_break = 0;  // 0 = none on this line
  float line = 0.0f;           // width of [start, i]
  for (std::size_t i = par.begin; i < par.end; ++i) {
    if (i > start && may_break_before(codes, i)) last_break = i;
    line += advances[i];
    if (width <= 0.0f || is_space(codes[i]) || line <= width) continue;
    const std::size_t cut = last_break > start ? last_break : std::max(i, start + 1);
    out.push_back({start, trim_end(codes, start, cut)});
    start = skip_spaces(codes, cut, par.end);
    if (start >= par.end) return;
    last_break = 0;
    line = 0.0f;
    i = start - 1;  // the loop goes on from the new line's start
  }
  out.push_back({start, trim_end(codes, start, par.end)});
}

}  // namespace

bool is_wide_script(char32_t c) {
  static constexpr std::array<std::pair<char32_t, char32_t>, 9> kRanges{{
      {0x1100, 0x11FF},   // Hangul Jamo
      {0x2E80, 0x303F},   // CJK radicals, punctuation
      {0x3040, 0x31FF},   // kana, bopomofo
      {0x3400, 0x4DBF},   // CJK extension A
      {0x4E00, 0x9FFF},   // CJK unified ideographs
      {0xAC00, 0xD7AF},   // Hangul syllables
      {0xF900, 0xFAFF},   // CJK compatibility ideographs
      {0xFF00, 0xFFEF},   // fullwidth forms
      {0x20000, 0x3FFFF}  // CJK extensions B..
  }};
  return std::any_of(kRanges.begin(), kRanges.end(), [c](const auto& r) { return c >= r.first && c <= r.second; });
}

bool may_break_before(const std::u32string& codes, std::size_t i) {
  const char32_t before = codes[i - 1];
  const char32_t c = codes[i];
  if (is_space(c)) return false;
  if (is_space(before)) return true;
  if (closes(c) || opens(before)) return false;
  return is_wide_script(before) || is_wide_script(c);
}

std::vector<LineRange> wrap_lines(const std::u32string& codes, const std::vector<float>& advances, float width) {
  std::vector<LineRange> out;
  std::size_t begin = 0;
  for (std::size_t i = 0; i <= codes.size(); ++i) {
    if (i < codes.size() && codes[i] != U'\n') continue;
    wrap_paragraph(codes, advances, width, {begin, i}, out);
    begin = i + 1;
  }
  return out;
}

}  // namespace alpha::core
