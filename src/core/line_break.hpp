#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Line breaking for wrapped text (pure), a small subset of UAX #14:
// - '\n' always ends a line;
// - a line may break after a space or tab (the spaces at its end are dropped);
// - a line may break between two ideographs / kana / Hangul / fullwidth forms
//   (CJK has no spaces), except before closing punctuation such as 。、）」
//   or small kana, and after opening punctuation such as （「;
// - a word wider than the line is cut where it overflows.
namespace alpha::core {

struct LineRange {
  std::size_t begin = 0;  // first code point of the line
  std::size_t end = 0;    // one past its last drawn code point
};

// True for code points written without spaces between words (CJK).
bool is_wide_script(char32_t c);
// True if a line may start at code point i of `codes` (0 < i < size).
bool may_break_before(const std::u32string& codes, std::size_t i);

// Lines of `codes` no wider than `width` (advances[i] = width of code point i;
// marks and ligature parts may be 0). width <= 0: only '\n' breaks.
std::vector<LineRange> wrap_lines(const std::u32string& codes, const std::vector<float>& advances, float width);

}  // namespace alpha::core
