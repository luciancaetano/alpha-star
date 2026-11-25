#pragma once

#include <array>
#include <cstddef>

#include "core/input.hpp"

namespace alpha::mcp {

// Inputs the agent injects, merged into each step's device frame so the game
// sees them exactly like real devices: a tap holds the inputs for one step,
// a hold until released. Per player (0-based).
class AgentInput {
 public:
  void tap(std::size_t player, const core::InputSet& inputs) { taps_[player] |= inputs; }
  void hold(std::size_t player, const core::InputSet& inputs) { held_[player] |= inputs; }
  void release(std::size_t player, const core::InputSet& inputs) { held_[player] &= ~inputs; }
  // Adds the injected inputs to `frame` (players with input count as
  // connected) and clears the taps.
  core::InputFrame apply(core::InputFrame frame);

 private:
  std::array<core::InputSet, core::kMaxPlayers> held_{};
  std::array<core::InputSet, core::kMaxPlayers> taps_{};
};

}  // namespace alpha::mcp
