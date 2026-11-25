#include "devtools/layout.hpp"

#include <algorithm>

namespace alpha::devtools {

namespace {

constexpr float kMargin = 8.0f;     // between windows and around them, at scale 1
constexpr float kTopBand = 100.0f;  // overlay / time height, at scale 1
constexpr float kConsoleShare = 0.25f;

enum class Axis { x, y };

// `r` cut in two along `axis` (the first part gets `share` of it), `gap` between them.
void split(const Rect& r, Axis axis, float share, float gap, Rect& a, Rect& b) {
  const float length = axis == Axis::x ? r.w : r.h;
  const float first = (length - gap) * share;
  const float second = length - first - gap;
  if (axis == Axis::x) {
    a = {r.x, r.y, first, r.h};
    b = {r.x + first + gap, r.y, second, r.h};
  } else {
    a = {r.x, r.y, r.w, first};
    b = {r.x, r.y + first + gap, r.w, second};
  }
}

}  // namespace

PanelLayout layout_panels(const Rect& area, float scale) {
  const float m = kMargin * scale;
  const Rect inner{area.x + m, area.y + m, std::max(area.w - 2 * m, 1.0f), std::max(area.h - 2 * m, 1.0f)};
  const float top = std::min(kTopBand * scale, inner.h / 4);
  Rect band;
  Rect rest;
  split(inner, Axis::y, top / inner.h, 0, band, rest);
  rest = {rest.x, rest.y + m, rest.w, std::max(rest.h - m, 1.0f)};
  PanelLayout out;
  split(band, Axis::x, 0.5f, m, out.overlay, out.time);
  Rect middle;
  split(rest, Axis::y, 1.0f - kConsoleShare, m, middle, out.console);
  const bool landscape = inner.w > inner.h;
  split(middle, landscape ? Axis::x : Axis::y, landscape ? 0.4f : 0.45f, m, out.entities, out.profiler);
  return out;
}

}  // namespace alpha::devtools
