#include "core/os.hpp"

#include <array>
#include <utility>

namespace alpha::core {

namespace {

constexpr std::array<std::pair<Os, std::string_view>, 7> kNames{{
    {Os::windows, "windows"}, {Os::macos, "macos"}, {Os::linux_os, "linux"}, {Os::android, "android"},
    {Os::ios, "ios"},         {Os::web, "web"},     {Os::other, "other"},
}};

constexpr std::array<std::pair<std::string_view, Os>, 7> kSdlNames{{
    {"Windows", Os::windows}, {"macOS", Os::macos}, {"Mac OS X", Os::macos}, {"Linux", Os::linux_os},
    {"Android", Os::android}, {"iOS", Os::ios},     {"Emscripten", Os::web},
}};

}  // namespace

Os os_from_sdl(std::string_view sdl_name) {
  for (const auto& [name, os] : kSdlNames) {
    if (name == sdl_name) return os;
  }
  return Os::other;
}

std::string_view os_name(Os os) {
  for (const auto& [value, name] : kNames) {
    if (value == os) return name;
  }
  return "other";
}

std::optional<Os> parse_os(std::string_view name) {
  for (const auto& [value, known] : kNames) {
    if (known == name) return value;
  }
  return std::nullopt;
}

}  // namespace alpha::core
