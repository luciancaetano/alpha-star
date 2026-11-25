#include "core/tilemap.hpp"

#include <cstddef>
#include <utility>

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

// Rects (indices into `out`) ending on the row above the current one, in x
// order like the runs, walked once per row: merging stays linear in the cells
// (a rect list scanned per run made a 1000 x 1000 checkerboard take minutes).
struct RowWalk {
  std::vector<std::size_t> above;
  std::vector<std::size_t> here;
  std::size_t next = 0;  // first rect of `above` not yet passed
  int row = 0;

  void start(int y) {
    above = y == row + 1 ? std::move(here) : std::vector<std::size_t>{};
    here.clear();
    next = 0;
    row = y;
  }

  // The rect the run continues (same span, ending right above), or out.size().
  std::size_t continued(const std::vector<Region>& out, const Region& run) {
    while (next < above.size() && out[above[next]].x < run.x) ++next;
    if (next < above.size() && out[above[next]].x == run.x && out[above[next]].w == run.w) return above[next];
    return out.size();
  }
};

}  // namespace

std::vector<Region> merge_cells(const std::vector<bool>& solid, int width) {
  std::vector<Region> out;
  if (width <= 0) return out;
  RowWalk walk;
  for (const Region& run : runs(solid, width)) {
    if (run.y != walk.row) walk.start(run.y);
    const std::size_t i = walk.continued(out, run);
    if (i == out.size()) out.push_back(run);
    else ++out[i].h;
    walk.here.push_back(i);
  }
  return out;
}

}  // namespace alpha::core
