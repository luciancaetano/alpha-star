#include <doctest/doctest.h>

#include <array>

#include "devtools/layout.hpp"

using alpha::devtools::layout_panels;
using alpha::devtools::PanelLayout;
using alpha::devtools::Rect;

namespace {

std::array<Rect, 5> all(const PanelLayout& l) { return {l.overlay, l.time, l.entities, l.profiler, l.console}; }

bool inside(const Rect& r, const Rect& area) {
  return r.x >= area.x && r.y >= area.y && r.x + r.w <= area.x + area.w + 0.01f && r.y + r.h <= area.y + area.h + 0.01f &&
         r.w > 0 && r.h > 0;
}

bool overlap(const Rect& a, const Rect& b) {
  return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

void check_fits(const Rect& area, float scale) {
  const auto rects = all(layout_panels(area, scale));
  for (std::size_t i = 0; i < rects.size(); ++i) {
    CHECK(inside(rects[i], area));
    for (std::size_t j = i + 1; j < rects.size(); ++j) CHECK_FALSE(overlap(rects[i], rects[j]));
  }
}

}  // namespace

TEST_CASE("layout: every panel inside the safe area, none overlapping (desktop and phone)") {
  check_fits({0, 0, 1280, 720}, 1.0f);
  check_fits({0, 0, 3840, 2160}, 2.0f);
  check_fits({0, 105, 1080, 2130}, 2.625f);  // phone: below the status bar
  check_fits({0, 0, 640, 360}, 1.0f);
}

TEST_CASE("layout: the safe area's origin is respected (status bar, notch)") {
  const PanelLayout l = layout_panels({0, 105, 1080, 2130}, 2.625f);
  CHECK(l.overlay.y > 105);
  CHECK(l.time.y > 105);
}

TEST_CASE("layout: landscape puts entities and profiler side by side, portrait stacks them") {
  const PanelLayout wide = layout_panels({0, 0, 1920, 1080}, 1.0f);
  CHECK(wide.profiler.x > wide.entities.x + wide.entities.w - 1);
  const PanelLayout tall = layout_panels({0, 0, 1080, 2340}, 2.625f);
  CHECK(tall.profiler.y > tall.entities.y + tall.entities.h - 1);
  CHECK(tall.profiler.w > 1000);  // the full width: its table is wide
}

TEST_CASE("layout: the console is the bottom strip, full width") {
  const Rect area{0, 0, 1920, 1080};
  const PanelLayout l = layout_panels(area, 1.0f);
  CHECK(l.console.y + l.console.h == doctest::Approx(area.h - 8));
  CHECK(l.console.w == doctest::Approx(area.w - 16));
}
