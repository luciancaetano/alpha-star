#pragma once

#include <string_view>

namespace alpha::core {

// Engine version string, e.g. "0.1.0". Exposed to scripts and MCP.
std::string_view version();

}  // namespace alpha::core
