#pragma once

#include <vector>

#include "core/image.hpp"

// Tile layer helpers (pure). Cells are row-major, `width` per row.
namespace alpha::core {

// Solid cells merged into few rectangles (in cells): horizontal runs first,
// then runs with the same span stacked downward. Every solid cell is in
// exactly one rect; no rect covers an empty cell.
std::vector<Region> merge_cells(const std::vector<bool>& solid, int width);

}  // namespace alpha::core
