#include <doctest/doctest.h>

#include "core/os.hpp"

using alpha::core::Os;

TEST_CASE("os: SDL platform names map to ctx.platform.os") {
  CHECK(alpha::core::os_from_sdl("Windows") == Os::windows);
  CHECK(alpha::core::os_from_sdl("macOS") == Os::macos);
  CHECK(alpha::core::os_from_sdl("Linux") == Os::linux_os);
  CHECK(alpha::core::os_from_sdl("Android") == Os::android);
  CHECK(alpha::core::os_from_sdl("iOS") == Os::ios);
  CHECK(alpha::core::os_from_sdl("Emscripten") == Os::web);
  CHECK(alpha::core::os_from_sdl("FreeBSD") == Os::other);
}

TEST_CASE("os: names round-trip; unknown names are rejected") {
  for (const Os os : {Os::windows, Os::macos, Os::linux_os, Os::android, Os::ios, Os::web, Os::other}) {
    CHECK(alpha::core::parse_os(alpha::core::os_name(os)) == os);
  }
  CHECK(alpha::core::os_name(Os::linux_os) == "linux");
  CHECK_FALSE(alpha::core::parse_os("Android").has_value());
  CHECK_FALSE(alpha::core::parse_os("").has_value());
}

TEST_CASE("os: phones and tablets are mobile") {
  CHECK(alpha::core::is_mobile(Os::android));
  CHECK(alpha::core::is_mobile(Os::ios));
  CHECK_FALSE(alpha::core::is_mobile(Os::windows));
  CHECK_FALSE(alpha::core::is_mobile(Os::web));
}
