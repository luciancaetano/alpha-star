#pragma once

#include <array>
#include <optional>

#include "devtools/layout.hpp"
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
void draw_panels(Panels& panels, TimeControl& time, script::Host& host, const DevSnapshot& snap,
                 const PanelLayout& layout);
// The next window opens at `r` (again each time the tools open: moved windows come back).
void place_next(const Rect& r);

}  // namespace alpha::devtools
