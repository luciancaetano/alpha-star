#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace alpha::core {

// The candidate closest to a misspelled `name` (edit distance at most a third
// of its length, at least 1), for "did you mean" hints; nullopt if none is close.
std::optional<std::string> closest(std::string_view name, std::span<const std::string> candidates);

}  // namespace alpha::core
