#include "core/suggest.hpp"

#include <algorithm>
#include <vector>

namespace alpha::core {

namespace {

std::size_t edit_distance(std::string_view a, std::string_view b) {
  std::vector<std::size_t> row(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j) row[j] = j;
  for (std::size_t i = 1; i <= a.size(); ++i) {
    std::size_t diagonal = row[0];
    row[0] = i;
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const std::size_t up = row[j];
      row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + (a[i - 1] == b[j - 1] ? 0 : 1)});
      diagonal = up;
    }
  }
  return row[b.size()];
}

}  // namespace

std::optional<std::string> closest(std::string_view name, std::span<const std::string> candidates) {
  const std::size_t limit = std::max<std::size_t>(1, name.size() / 3);
  std::optional<std::string> best;
  std::size_t best_distance = limit + 1;
  for (const std::string& c : candidates) {
    const std::size_t d = edit_distance(name, c);
    if (d < best_distance) {
      best = c;
      best_distance = d;
    }
  }
  return best;
}

}  // namespace alpha::core
