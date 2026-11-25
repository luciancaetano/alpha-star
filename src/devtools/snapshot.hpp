#pragma once

#include <cstddef>
#include <string>

#include "script/host.hpp"

namespace alpha::devtools {

// Measured by the main loop for one rendered frame.
struct FrameStats {
  double frame_ms = 0.0;  // real time since the previous frame
  int steps = 0;          // fixed steps run this frame
};

// Everything the dev tools overlay shows, read once per frame.
struct DevSnapshot {
  double fps = 0.0;
  double frame_ms = 0.0;
  int steps = 0;
  double call_ms = 0.0;    // last script call
  double budget_ms = 0.0;  // per-call limit (script::kCallBudget)
  std::size_t entities = 0;
  std::size_t voices = 0;
  std::string scene;  // empty before the first scene starts
  std::string error;  // the pausing script error, if any
};

DevSnapshot snapshot(script::Host& host, FrameStats stats);

}  // namespace alpha::devtools
