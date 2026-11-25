#include "core/tilemap.hpp"

#include <cstddef>

namespace alpha::core {

namespace {

// Horizontal runs of solid cells, row by row, as 1-cell-tall regions.
std::vector<Region> runs(const std::vector<bool>& solid, int width) {
  std::vector<Region> out;
  const int rows = static_cast<int>(solid.size()) / width;
  for (int y = 0; y < rows; ++y) {
    int start = -1;
    for (int x = 0; x <= width; ++x) {
      const bool on = x < width && solid[static_cast<std::size_t>(y * width + x)];
      if (on && start < 0) start = x;
      if (!on && start >= 0) {
        out.push_back({start, y, x - start, 1});
        start = -1;
      }
    }
  }
  return out;
}

// The open rect a run continues: same span, ending on the row above.
Region* continued(std::vector<Region>& open, const Region& run) {
  for (Region& r : open) {
    if (r.x == run.x && r.w == run.w && r.y + r.h == run.y) return &r;
  }
  return nullptr;
}

}  // namespace

std::vector<Region> merge_cells(const std::vector<bool>& solid, int width) {
  std::vector<Region> out;
  if (width <= 0) return out;
  for (const Region& run : runs(solid, width)) {
    if (Region* r = continued(out, run)) ++r->h;
    else out.push_back(run);
  }
  return out;
}

}  // namespace alpha::core
