#include <doctest/doctest.h>

#if ALPHA_DEBUG_TOOLS  // hot reload exists only in dev/release builds

#include <string>

#include "host_fixture.hpp"
#include "script/hot_reload.hpp"

using alpha::script::HotReload;
using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

std::string game(const std::string& tag) {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { enter() { console.log('" + tag + "'); } });\n"
         "export default game({ config: { title: 't', size: [100, 100] }, start: 'play', scenes: { play } });\n";
}

std::string last_log(const Host& host) { return host.log().at(host.log().size() - 1).text; }

}  // namespace

TEST_CASE("hot reload: a changed pack reloads the game once it is quiet; a broken one keeps it") {
  Sandbox box;
  Host host(box.save());
  box.write("main.js", game("v1"));
  host.load(box.dir.string());
  HotReload watch(host, true);
  box.write("main.js", game("v2"));
  watch.poll(host, 10.0);
  CHECK(last_log(host) == "v1");
  watch.poll(host, 11.0);
  CHECK(last_log(host) == "v2");
  box.write("main.js", "export default 1 +;");
  watch.poll(host, 12.0);
  watch.poll(host, 13.0);
  CHECK(host.reload_error().find("previous game keeps running") != std::string::npos);
  CHECK(host.scene() == "play");
}

TEST_CASE("hot reload: disabled never watches") {
  Sandbox box;
  Host host(box.save());
  box.write("main.js", game("v1"));
  host.load(box.dir.string());
  HotReload watch(host, false);
  box.write("main.js", game("v2"));
  watch.poll(host, 10.0);
  watch.poll(host, 11.0);
  CHECK(last_log(host) == "v1");
}

#endif
