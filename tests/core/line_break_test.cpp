#include <doctest/doctest.h>

#include <vector>

#include "core/line_break.hpp"
#include "core/utf8.hpp"

namespace {

// Lines of `text` with every code point 10 wide.
std::vector<std::u32string> wrap(const char* text, float width) {
  const std::u32string codes = alpha::core::decode_utf8(text);
  std::vector<std::u32string> out;
  for (const auto& r : alpha::core::wrap_lines(codes, std::vector<float>(codes.size(), 10.0f), width)) {
    out.push_back(codes.substr(r.begin, r.end - r.begin));
  }
  return out;
}

using Lines = std::vector<std::u32string>;

}  // namespace

TEST_CASE("wrapping breaks at spaces and drops them; a newline always breaks") {
  CHECK(wrap("one two three", 80) == Lines{U"one two", U"three"});
  CHECK(wrap("one two", 0) == Lines{U"one two"});
  CHECK(wrap("a\n\nb", 0) == Lines{U"a", U"", U"b"});
  CHECK(wrap("abcdefgh", 30) == Lines{U"abc", U"def", U"gh"});  // too long: cut
}

TEST_CASE("CJK breaks between ideographs, never before closing punctuation") {
  // 你好世界, 你好。世界, 「你好」
  CHECK(wrap("\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c", 20) == Lines{U"你好", U"世界"});
  CHECK(wrap("\xe4\xbd\xa0\xe5\xa5\xbd\xe3\x80\x82\xe4\xb8\x96\xe7\x95\x8c", 20) ==
        Lines{U"你", U"好。", U"世界"});
  CHECK(wrap("\xe3\x80\x8c\xe4\xbd\xa0\xe5\xa5\xbd\xe3\x80\x8d", 30) == Lines{U"「你", U"好」"});
  CHECK(alpha::core::is_wide_script(0x3042));  // hiragana
  CHECK_FALSE(alpha::core::is_wide_script(U'a'));
}
