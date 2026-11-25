#pragma once

#include <algorithm>
#include <iterator>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
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

// The strings of `parts` with `sep` between them.
template <typename Range>
std::string join(const Range& parts, std::string_view sep) {
  std::string out;
  for (const auto& p : parts) out += (out.empty() ? std::string() : std::string(sep)) + p;
  return out;
}

// A map's keys with `sep` between them (for error messages listing what exists).
template <typename Map>
std::string keys_of(const Map& map, std::string_view sep) {
  std::vector<std::string> keys;
  for (const auto& entry : map) keys.push_back(entry.first);
  return join(keys, sep);
}

// Every element's `.name`, space separated (for error messages).
template <typename Range>
std::string names_of(const Range& range) {
  std::string out;
  for (const auto& e : range) out += (out.empty() ? "" : " ") + e.name;
  return out;
}

}  // namespace alpha::core
