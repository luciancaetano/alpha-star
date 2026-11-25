#pragma once

#include <string>
#include <string_view>

namespace alpha::mcp {

// Standard base64 (RFC 4648, with padding) of raw bytes.
std::string base64(std::string_view bytes);

}  // namespace alpha::mcp
