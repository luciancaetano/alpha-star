#pragma once

#include <algorithm>
#include <iterator>
#include <optional>

namespace alpha::core {

// Position of `value` in `range` as id type `Id`; nullopt if absent.
template <typename Id, typename Range, typename T>
std::optional<Id> index_of(const Range& range, const T& value) {
  const auto it = std::find(std::begin(range), std::end(range), value);
  if (it == std::end(range)) return std::nullopt;
  return static_cast<Id>(std::distance(std::begin(range), it));
}

}  // namespace alpha::core
