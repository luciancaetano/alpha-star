#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace alpha::core {

struct Color {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;
  bool operator==(const Color&) const = default;
};

// Parses "#rgb", "#rrggbb" or "#rrggbbaa". nullopt if malformed.
std::optional<Color> parse_color(std::string_view text);
// r, g, b multiplied by k (0..1, rounded); alpha kept.
Color scaled(Color c, float k);

}  // namespace alpha::core
