#include "core/tags.hpp"

#include "core/error.hpp"
#include "core/index_of.hpp"

namespace alpha::core {

TagId TagRegistry::intern(std::string_view name) {
  if (const auto id = find(name)) return *id;
  if (names_.size() >= kMaxTags) {
    throw Error("too many tags (max " + std::to_string(kMaxTags) + "); cannot add \"" +
                std::string(name) + "\"");
  }
  names_.emplace_back(name);
  return static_cast<TagId>(names_.size() - 1);
}

std::optional<TagId> TagRegistry::find(std::string_view name) const {
  return index_of<TagId>(names_, name);
}

}  // namespace alpha::core
