#include <doctest/doctest.h>

#include <filesystem>
#include <string>

#include "core/log_ring.hpp"
#include "platform/log_file.hpp"

namespace fs = std::filesystem;
using alpha::core::LogLevel;
using alpha::core::LogRing;

TEST_CASE("per-user files live in the pref dir named after the game title") {
  const std::string path = alpha::platform::pref_file("alpha log test", "log.txt");
  REQUIRE_FALSE(path.empty());
  CHECK(fs::path(path).filename() == "log.txt");
  CHECK(fs::path(path).parent_path().filename() == "alpha log test");
  CHECK(fs::is_directory(fs::path(path).parent_path()));
}

TEST_CASE("lines logged before the log file opened are written to it first, oldest first") {
  LogRing ring(10);
  ring.push(LogLevel::Info, "booting");
  ring.push(LogLevel::Error, "boom");
  CHECK(alpha::platform::text_of(ring) == "[log] booting\n[error] boom\n");
}
