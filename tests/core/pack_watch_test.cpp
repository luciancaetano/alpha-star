#include <doctest/doctest.h>

#if ALPHA_DEBUG_TOOLS  // the pack watcher exists only in dev/release builds

#include <string>

#include "core/pack_watch.hpp"

using alpha::core::PackWatch;

namespace {

// A pack seen as the file layer lists it: path -> (size, modified).
struct Pack {
  PackWatch::Snapshot files{{"main.js", {1, 100}}};
  PackWatch::Scan scan() {
    return [this] { return files; };
  }
  void write(const std::string& name, std::uint64_t size) { files[name] = {size, files[name].second + 1}; }
};

}  // namespace

TEST_CASE("pack watch: nothing changed, nothing to reload") {
  Pack pack;
  PackWatch watch(pack.scan());
  for (double t = 0; t < 3; t += 0.1) CHECK_FALSE(watch.poll(t));
}

TEST_CASE("pack watch: a change reloads once, after the pack has been quiet") {
  Pack pack;
  PackWatch watch(pack.scan());
  pack.write("main.js", 2);
  CHECK_FALSE(watch.poll(1.0));   // seen, waiting for quiet
  CHECK_FALSE(watch.poll(1.1));   // no scan before the poll interval
  CHECK(watch.poll(1.3));         // quiet for the debounce time
  CHECK_FALSE(watch.poll(1.6));
  CHECK_FALSE(watch.poll(5.0));
}

TEST_CASE("pack watch: an export writing several files over time reloads once") {
  Pack pack;
  PackWatch watch(pack.scan());
  pack.write("main.js", 2);
  CHECK_FALSE(watch.poll(1.0));
  pack.write("assets/hud.rml", 6);  // a new file in a subfolder
  CHECK_FALSE(watch.poll(1.25));
  pack.write("main.js.map", 2);
  CHECK_FALSE(watch.poll(1.5));
  CHECK_FALSE(watch.poll(1.75));  // 0.25 s since the last change
  CHECK(watch.poll(2.0));
  CHECK_FALSE(watch.poll(2.25));
}

TEST_CASE("pack watch: a touched file with the same size is a change") {
  Pack pack;
  PackWatch watch(pack.scan());
  pack.write("main.js", 1);
  CHECK_FALSE(watch.poll(1.0));
  CHECK(watch.poll(1.5));
}

TEST_CASE("pack watch: deleting a file is a change") {
  Pack pack;
  pack.write("old.png", 1);
  PackWatch watch(pack.scan());
  pack.files.erase("old.png");
  CHECK_FALSE(watch.poll(1.0));
  CHECK(watch.poll(1.5));
}

#endif
