#include "core/utf8.hpp"

namespace alpha::core {

namespace {

constexpr char32_t kReplacement = 0xFFFD;

// One decoded sequence: the code point and its byte length, or why it is invalid.
struct Step {
  char32_t code = 0;
  std::size_t length = 1;
  const char* fault = nullptr;
};

std::size_t sequence_length(unsigned char lead) {
  if (lead < 0x80) return 1;
  if (lead >= 0xC2 && lead <= 0xDF) return 2;
  if ((lead & 0xF0) == 0xE0) return 3;
  if (lead >= 0xF0 && lead <= 0xF4) return 4;
  return 0;
}

// The shortest form only, no surrogates, nothing past U+10FFFF.
const char* range_fault(char32_t c, std::size_t n) {
  if ((n == 3 && c < 0x800) || (n == 4 && c < 0x10000)) return "overlong encoding";
  if (c >= 0xD800 && c <= 0xDFFF) return "a surrogate (an unpaired UTF-16 half, e.g. one half of an emoji)";
  if (c > 0x10FFFF) return "a code point past U+10FFFF";
  return nullptr;
}

Step step(std::string_view text, std::size_t i) {
  const auto lead = static_cast<unsigned char>(text[i]);
  const std::size_t n = sequence_length(lead);
  if (n == 0) return {kReplacement, 1, "a byte that cannot start a character"};
  if (i + n > text.size()) return {kReplacement, 1, "a character cut short at the end"};
  char32_t c = n == 1 ? lead : static_cast<char32_t>(lead & (0x7F >> n));
  for (std::size_t k = 1; k < n; ++k) {
    const auto b = static_cast<unsigned char>(text[i + k]);
    if ((b & 0xC0) != 0x80) return {kReplacement, 1, "a character cut short"};
    c = (c << 6) | (b & 0x3F);
  }
  if (const char* why = range_fault(c, n)) return {kReplacement, 1, why};
  return {c, n, nullptr};
}

}  // namespace

std::optional<Utf8Fault> utf8_fault(std::string_view text) {
  for (std::size_t i = 0; i < text.size();) {
    const Step s = step(text, i);
    if (s.fault) return Utf8Fault{i, s.fault};
    i += s.length;
  }
  return std::nullopt;
}

std::u32string decode_utf8(std::string_view text) {
  std::u32string out;
  out.reserve(text.size());
  for (std::size_t i = 0; i < text.size();) {
    const Step s = step(text, i);
    out.push_back(s.code);
    i += s.length;
  }
  return out;
}

void append_utf8(std::string& out, char32_t c) {
  if (c < 0x80) {
    out.push_back(static_cast<char>(c));
    return;
  }
  const int n = c < 0x800 ? 2 : c < 0x10000 ? 3 : 4;
  const unsigned lead = n == 2 ? 0xC0u : n == 3 ? 0xE0u : 0xF0u;
  out.push_back(static_cast<char>(lead | (c >> (6 * (n - 1)))));
  for (int k = n - 2; k >= 0; --k) out.push_back(static_cast<char>(0x80u | ((c >> (6 * k)) & 0x3Fu)));
}

}  // namespace alpha::core
