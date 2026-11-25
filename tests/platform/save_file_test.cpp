#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

#include "core/error.hpp"
#include "platform/save_file.hpp"

using namespace alpha;
namespace fs = std::filesystem;

namespace {

fs::path fresh_dir(const char* name) {
  const fs::path dir = fs::temp_directory_path() / name;
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

}  // namespace

TEST_CASE("read_file gives the text, or nothing when the file is missing") {
  const fs::path dir = fresh_dir("alpha_save_read");
  CHECK_FALSE(platform::read_file(dir / "none.json").has_value());
  std::ofstream(dir / "a.json") << "{\"x\":1}";
  CHECK(platform::read_file(dir / "a.json") == std::optional<std::string>("{\"x\":1}"));
  fs::remove_all(dir);
}

TEST_CASE("write_atomic replaces the file and keeps the previous one as .bak") {
  const fs::path dir = fresh_dir("alpha_save_write");
  const fs::path file = dir / "save.json";
  platform::write_atomic(file, "one");
  CHECK(platform::read_file(file) == std::optional<std::string>("one"));
  CHECK_FALSE(fs::exists(dir / "save.json.bak"));
  platform::write_atomic(file, "two");
  CHECK(platform::read_file(file) == std::optional<std::string>("two"));
  CHECK(platform::read_file(dir / "save.json.bak") == std::optional<std::string>("one"));
  CHECK_FALSE(fs::exists(dir / "save.json.tmp"));
  fs::remove_all(dir);
}

TEST_CASE("write_atomic names the file when it cannot write") {
  const fs::path missing = fs::temp_directory_path() / "alpha_no_such_dir" / "x" / "save.json";
  fs::remove_all(fs::temp_directory_path() / "alpha_no_such_dir");
  CHECK_THROWS_WITH_AS(platform::write_atomic(missing, "x"), doctest::Contains("save.json"), core::Error);
}
