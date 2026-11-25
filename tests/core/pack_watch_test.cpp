#include <doctest/doctest.h>

#if ALPHA_DEBUG_TOOLS  // the pack watcher exists only in dev/release builds

#include <filesystem>
#include <fstream>
#include <string>

#include "core/pack_watch.hpp"

using alpha::core::PackWatch;
namespace fs = std::filesystem;

namespace {

struct Dir {
  fs::path path = fs::temp_directory_path() / "alpha_pack_watch_test";
  Dir() {
    fs::remove_all(path);
    fs::create_directories(path / "assets");
    write("main.js", "1");
  }
  ~Dir() { fs::remove_all(path); }
  Dir(const Dir&) = delete;
  Dir& operator=(const Dir&) = delete;
  void write(const std::string& name, const std::string& text) const { std::ofstream(path / name) << text; }
};

}  // namespace

TEST_CASE("pack watch: nothing changed, nothing to reload") {
  Dir dir;
  PackWatch watch(dir.path);
  for (double t = 0; t < 3; t += 0.1) CHECK_FALSE(watch.poll(t));
}

TEST_CASE("pack watch: a change reloads once, after the pack has been quiet") {
  Dir dir;
  PackWatch watch(dir.path);
  dir.write("main.js", "22");
  CHECK_FALSE(watch.poll(1.0));   // seen, waiting for quiet
  CHECK_FALSE(watch.poll(1.1));   // no scan before the poll interval
  CHECK(watch.poll(1.3));         // quiet for the debounce time
  CHECK_FALSE(watch.poll(1.6));
  CHECK_FALSE(watch.poll(5.0));
}

TEST_CASE("pack watch: an export writing several files over time reloads once") {
  Dir dir;
  PackWatch watch(dir.path);
  dir.write("main.js", "22");
  CHECK_FALSE(watch.poll(1.0));
  dir.write("assets/hud.rml", "<rml/>");  // a new file in a subfolder
  CHECK_FALSE(watch.poll(1.25));
  dir.write("main.js.map", "{}");
  CHECK_FALSE(watch.poll(1.5));
  CHECK_FALSE(watch.poll(1.75));  // 0.25 s since the last change
  CHECK(watch.poll(2.0));
  CHECK_FALSE(watch.poll(2.25));
}

TEST_CASE("pack watch: deleting a file is a change") {
  Dir dir;
  dir.write("old.png", "x");
  PackWatch watch(dir.path);
  fs::remove(dir.path / "old.png");
  CHECK_FALSE(watch.poll(1.0));
  CHECK(watch.poll(1.5));
}

TEST_CASE("pack watch: a missing directory never reloads") {
  PackWatch watch(fs::temp_directory_path() / "alpha_pack_watch_missing");
  CHECK_FALSE(watch.poll(1.0));
  CHECK_FALSE(watch.poll(2.0));
}

#endif
