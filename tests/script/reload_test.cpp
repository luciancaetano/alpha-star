#include <doctest/doctest.h>

#if ALPHA_DEBUG_TOOLS  // hot reload exists only in dev/release builds

#include <string>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// title goes to play({level: 3}) on the first step; play counts steps in
// ctx.state.n and logs "enter <tag> n=<n> level=<level> save=<k>" on enter.
std::string game(const std::string& tag, const std::string& state = "state: (p) => ({ n: 0, level: p.level }),",
                 const std::string& extra = "") {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const title = scene('title', { update(ctx) { ctx.go('play', { level: 3 }); } });\n"
         "const play = scene('play', { " + state + "\n"
         "  enter(ctx, p) { " + extra + " console.log(`enter " + tag +
         " n=${ctx.state?.n} level=${p?.level} save=${ctx.save.get('k', 0)}`); },\n"
         "  update(ctx) { ctx.state.n += 1; } });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 1 },\n"
         "  start: 'title', scenes: { title, play }, assets: { sounds: {} } });\n";
}

std::string last_log(const Host& host) { return host.log().at(host.log().size() - 1).text; }

void steps(Host& host, int n) {
  for (int i = 0; i < n; ++i) host.step({}, kDt);
}

// A game loaded from box/game.js and run into play with n = 5.
struct Rig {
  Sandbox box;
  Host host{box.save()};
  Rig() {
    host.load(box.write("game.js", game("v1")));
    steps(host, 6);  // step 1 enters play, 5 more count
  }
  std::string reload_with(const std::string& source) {
    box.write("game.js", source);
    return host.reload();
  }
};

}  // namespace

TEST_CASE("reload: keeps the scene, its params and a JSON state") {
  Rig rig;
  REQUIRE(last_log(rig.host) == "enter v1 n=0 level=3 save=0");
  CHECK(rig.reload_with(game("v2")) == "");
  CHECK(rig.host.scene() == "play");
  CHECK(last_log(rig.host) == "enter v2 n=5 level=3 save=0");
  steps(rig.host, 1);
  CHECK_FALSE(rig.host.paused());
}

TEST_CASE("reload: keeps the save") {
  Rig rig;
  CHECK(rig.reload_with(game("v2", "state: (p) => ({ n: 0, level: p.level }),", "ctx.save.set('k', 7);")) == "");
  CHECK(rig.reload_with(game("v3")) == "");
  CHECK(last_log(rig.host) == "enter v3 n=5 level=3 save=7");
}

TEST_CASE("reload: a state that does not round-trip through JSON is rebuilt by state(params)") {
  Rig rig;
  CHECK(rig.reload_with(game("v2", "state: (p) => ({ n: 0, level: p.level }),", "ctx.state.f = () => 1;")) == "");
  steps(rig.host, 2);
  CHECK(rig.reload_with(game("v3")) == "");
  CHECK(last_log(rig.host) == "enter v3 n=0 level=3 save=0");
}

TEST_CASE("reload: a scene that no longer declares state gets none") {
  Rig rig;
  CHECK(rig.reload_with(game("v2", "")).empty());
  CHECK(last_log(rig.host) == "enter v2 n=undefined level=3 save=0");
}

TEST_CASE("reload: a removed scene restarts the game at its start scene") {
  Rig rig;
  const std::string only_title =
      "import { define } from 'alpha'; const { scene, game } = define();\n"
      "const menu = scene('menu', { enter() { console.log('menu'); } });\n"
      "export default game({ config: { title: 't', size: [100, 100] }, start: 'menu', scenes: { menu } });\n";
  CHECK(rig.reload_with(only_title) == "");
  CHECK(rig.host.scene() == "menu");
  CHECK(last_log(rig.host) == "menu");
}

TEST_CASE("reload: a syntax error keeps the old game running and shows the error until a reload works") {
  Rig rig;
  const std::string error = rig.reload_with("export default game({");
  CHECK(error.find("previous game keeps running") != std::string::npos);
  CHECK(error.find("SyntaxError") != std::string::npos);
  CHECK(rig.host.reload_error() == error);
  CHECK(last_log(rig.host) == error);
  CHECK(rig.host.scene() == "play");
  CHECK_FALSE(rig.host.paused());
  CHECK_FALSE(rig.host.frame().texts.empty());
  steps(rig.host, 2);  // the old code still counts: n = 7
  CHECK(rig.reload_with(game("v2")) == "");
  CHECK(last_log(rig.host) == "enter v2 n=7 level=3 save=0");
  CHECK(rig.host.reload_error().empty());
  CHECK(rig.host.frame().texts.empty());
}

TEST_CASE("reload: a runtime error while entering the new scene keeps the old game") {
  Rig rig;
  const std::string error = rig.reload_with(game("v2", "state: (p) => ({ n: 0 }),", "throw new Error('boom');"));
  CHECK(error.find("boom") != std::string::npos);
  CHECK(rig.host.scene() == "play");
  steps(rig.host, 1);
  CHECK(rig.reload_with(game("v3")) == "");
  CHECK(last_log(rig.host) == "enter v3 n=6 level=3 save=0");
}

TEST_CASE("reload: fixes a game paused by a script error") {
  Rig rig;
  CHECK(rig.reload_with(game("bad", "state: (p) => ({ n: 0, level: p.level }),",
                             "ctx.after(0.01, () => { throw new Error('crash'); });")) == "");
  steps(rig.host, 3);
  REQUIRE(rig.host.paused());
  CHECK(rig.reload_with(game("fixed")) == "");
  CHECK_FALSE(rig.host.paused());
  CHECK(rig.host.scene() == "play");
  steps(rig.host, 3);
  CHECK_FALSE(rig.host.paused());
}

TEST_CASE("reload: picks up changed assets") {
  Sandbox box;
  Host host(box.save());
  const auto source = [] {
    return std::string(
        "import { define } from 'alpha'; const { scene, game } = define();\n"
        "const play = scene('play', { enter(ctx) { ctx.ui.show('hud'); } });\n"
        "export default game({ config: { title: 't', size: [100, 100] }, start: 'play', scenes: { play },\n"
        "  assets: { ui: { hud: 'hud.rml' } } });\n");
  };
  box.write("hud.rml", "<rml><head></head><body><p id=\"a\">one</p></body></rml>");
  host.load(box.write("game.js", source()));
  REQUIRE(alpha::test::ui_text(host, "a") == "one");
  box.write("hud.rml", "<rml><head></head><body><p id=\"a\">two</p></body></rml>");
  CHECK(host.reload() == "");
  CHECK(alpha::test::ui_text(host, "a") == "two");
  host.release_ui();
}

TEST_CASE("reload: a pack directory reloads its main.js; pack_dir is what to watch") {
  Sandbox box;
  Host host(box.save());
  box.write("main.js", game("v1"));
  host.load(box.dir.string());
  CHECK(host.pack_dir() == box.dir);
  steps(host, 2);
  box.write("main.js", game("v2"));
  CHECK(host.reload() == "");
  CHECK(last_log(host) == "enter v2 n=1 level=3 save=0");
}

#endif
