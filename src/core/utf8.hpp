#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

// UTF-8 text as Unicode code points (pure). Text from scripts is checked once
// where it enters (script::text_arg), so everything inside holds valid UTF-8.
namespace alpha::core {

// Where `text` stops being valid UTF-8 (overlong forms, surrogates such as an
// unpaired JS surrogate, values past U+10FFFF, truncated sequences), if anywhere.
struct Utf8Fault {
  std::size_t offset = 0;  // byte offset of the bad sequence
  std::string why;
};
std::optional<Utf8Fault> utf8_fault(std::string_view text);

// The code points of `text`; each invalid sequence becomes U+FFFD.
std::u32string decode_utf8(std::string_view text);

// Appends code point `c` (a Unicode scalar value) as UTF-8.
void append_utf8(std::string& out, char32_t c);

}  // namespace alpha::core
