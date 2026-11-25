#pragma once

#include <bitset>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace alpha::core {

using TagId = std::uint8_t;
inline constexpr std::size_t kMaxTags = 64;
using TagSet = std::bitset<kMaxTags>;

// Interns tag names to small ids so entities carry a bitset, not strings.
class TagRegistry {
 public:
  // Throws Error once kMaxTags distinct names exist.
  TagId intern(std::string_view name);
  std::optional<TagId> find(std::string_view name) const;
  std::string_view name(TagId id) const { return names_[id]; }
  std::size_t size() const { return names_.size(); }

 private:
  std::vector<std::string> names_;
};

}  // namespace alpha::core
