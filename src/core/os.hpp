#pragma once

#include <optional>
#include <string_view>

namespace alpha::core {

// The OS a game runs on (ctx.platform.os): games adapt hints and layout to it.
// `linux_os` because GCC predefines `linux` as a macro.
enum class Os { windows, macos, linux_os, android, ios, web, other };

// SDL_GetPlatform() names ("Windows", "macOS", "Linux", "Android", "iOS", "Emscripten").
Os os_from_sdl(std::string_view sdl_name);
// The script name ("windows", "linux", "android", ...) and back; nullopt when unknown.
std::string_view os_name(Os os);
std::optional<Os> parse_os(std::string_view name);

// Phones and tablets: touch first, no keyboard.
constexpr bool is_mobile(Os os) { return os == Os::android || os == Os::ios; }

}  // namespace alpha::core
