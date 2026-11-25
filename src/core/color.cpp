#include "core/color.hpp"

#include <array>
#include <string>

namespace alpha::core {

namespace {

std::optional<std::uint8_t> hex_digit(char c) {
  if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
  if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(c - 'a' + 10);
  if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(c - 'A' + 10);
  return std::nullopt;
}

// "abc" -> "aabbccff", "aabbcc" -> "aabbccff"; other lengths unchanged.
std::string expand(std::string_view hex) {
  if (hex.size() == 3) return {hex[0], hex[0], hex[1], hex[1], hex[2], hex[2], 'f', 'f'};
  if (hex.size() == 6) return std::string(hex) + "ff";
  return std::string(hex);
}

}  // namespace

std::optional<Color> parse_color(std::string_view text) {
  if (text.empty() || text[0] != '#') return std::nullopt;
  const std::string hex = expand(text.substr(1));
  if (hex.size() != 8) return std::nullopt;
  std::array<std::uint8_t, 4> ch{};
  for (std::size_t i = 0; i < hex.size(); ++i) {
    const auto digit = hex_digit(hex[i]);
    if (!digit) return std::nullopt;
    ch[i / 2] = static_cast<std::uint8_t>(ch[i / 2] * 16 + *digit);
  }
  return Color{ch[0], ch[1], ch[2], ch[3]};
}

}  // namespace alpha::core
