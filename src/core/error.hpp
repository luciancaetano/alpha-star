#pragma once

#include <stdexcept>

namespace alpha::core {

// Raised when outside data (script, file, MCP) is invalid. The message is
// written for the agent that will fix it: what failed and what was available.
struct Error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

}  // namespace alpha::core
