#pragma once

#include <algorithm>
#include <iterator>
#include <cstddef>
#include <optional>
#include <string_view>

namespace alpha::core {

// Position of `value` in `range` as id type `Id`; nullopt if absent.
template <typename Id, typename Range, typename T>
std::optional<Id> index_of(const Range& range, const T& value) {
  const auto it = std::find(std::begin(range), std::end(range), value);
  if (it == std::end(range)) return std::nullopt;
  return static_cast<Id>(std::distance(std::begin(range), it));
}

// Position of the first element whose `.name` equals `name`; nullopt if none.
template <typename Id, typename Range>
std::optional<Id> index_by_name(const Range& range, std::string_view name) {
  for (std::size_t i = 0; i < range.size(); ++i) {
    if (range[i].name == name) return static_cast<Id>(i);
  }
  return std::nullopt;
}

}  // namespace alpha::core
