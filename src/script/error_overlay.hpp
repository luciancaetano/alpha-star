#pragma once

#include <string>
#include <vector>

#include "core/entity_spec.hpp"

namespace alpha::script {

// Splits `text` into lines of at most `columns` characters, breaking at
// newlines and (when possible) spaces.
std::vector<std::string> wrap(const std::string& text, std::size_t columns);

// Error message as small red text lines filling a `width`-pixel screen.
std::vector<core::TextItem> error_overlay(const std::string& error, float width);

}  // namespace alpha::script
