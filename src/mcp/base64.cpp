#include "mcp/base64.hpp"

#include <algorithm>
#include <cstdint>

namespace alpha::mcp {

std::string base64(std::string_view bytes) {
  static constexpr char kDigits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve((bytes.size() + 2) / 3 * 4);
  for (std::size_t i = 0; i < bytes.size(); i += 3) {
    const std::size_t n = std::min<std::size_t>(3, bytes.size() - i);
    std::uint32_t group = 0;
    for (std::size_t k = 0; k < 3; ++k) {
      group = (group << 8) | (k < n ? static_cast<unsigned char>(bytes[i + k]) : 0U);
    }
    for (std::size_t k = 0; k < 4; ++k) out += k <= n ? kDigits[(group >> (18 - 6 * k)) & 0x3F] : '=';
  }
  return out;
}

}  // namespace alpha::mcp
