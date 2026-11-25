#include "mcp/agent_input.hpp"

#include <algorithm>

namespace alpha::mcp {

core::InputFrame AgentInput::apply(core::InputFrame frame) {
  for (std::size_t p = 0; p < core::kMaxPlayers; ++p) {
    const core::InputSet injected = held_[p] | taps_[p];
    frame.players[p].held |= injected;
    if (injected.any()) frame.connected = std::max(frame.connected, p + 1);
    taps_[p].reset();
  }
  return frame;
}

}  // namespace alpha::mcp
