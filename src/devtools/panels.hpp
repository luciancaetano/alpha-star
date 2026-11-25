#pragma once

#include <array>
#include <optional>

#include "devtools/snapshot.hpp"
#include "devtools/time_control.hpp"
#include "script/host.hpp"

namespace alpha::devtools {

// UI state of the dev tools windows (what the user picked, not game state).
struct Panels {
  bool physics_view = false;
  std::array<char, 64> filter{};  // entity inspector tag filter
  std::optional<ecs::EntityId> selected;
};

// One ImGui frame's windows: overlay, time controls, inspector, console.
// Must run between ImGui::NewFrame and ImGui::Render.
void draw_panels(Panels& panels, TimeControl& time, script::Host& host, const DevSnapshot& snap);

}  // namespace alpha::devtools
