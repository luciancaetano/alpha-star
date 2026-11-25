#pragma once

namespace alpha::devtools {

struct Rect {
  float x = 0;
  float y = 0;
  float w = 0;
  float h = 0;
};

// Where each dev tools window opens.
struct PanelLayout {
  Rect overlay;
  Rect time;
  Rect entities;
  Rect profiler;
  Rect console;
};

// The windows tiled inside `area` (the window's safe area: below a status bar
// or notch) with margins of 8 * scale: overlay and time across the top, the
// console along the bottom, entities and profiler in between (side by side in
// landscape, stacked in portrait). Pure.
PanelLayout layout_panels(const Rect& area, float scale);

}  // namespace alpha::devtools
