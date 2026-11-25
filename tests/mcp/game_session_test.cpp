#include <doctest/doctest.h>

#include "core/error.hpp"
#include "mcp/game_session.hpp"
#include "mcp_fixture.hpp"

using alpha::core::Error;
using alpha::mcp::GameSession;
using alpha::mcp::Press;

namespace {

struct Rig {
  alpha::test::McpGame game;
  GameSession session{game.host, game.renderer, [] { return alpha::core::InputFrame{}; }, alpha::test::kStepDt};
  // Clicks Start and runs the step that handles it.
  void to_play() {
    session.click("menu", "start");
    session.set_paused(true);
    session.step(1);
  }
};

std::string message_of(auto&& fn) {
  try {
    fn();
  } catch (const Error& e) {
    return e.what();
  }
  return "";
}

}  // namespace

TEST_CASE("session: state shows the scene, UI documents, entities and the frame count") {
  Rig rig;
  REQUIRE_MESSAGE(!rig.game.host.paused(), rig.game.host.error());
  auto s = rig.session.state();
  CHECK(s.scene == "title");
  CHECK(s.ui.at("menu").shown);
  CHECK(s.ui.at("menu").texts.at("start") == "Start");
  CHECK(s.entities.empty());
  CHECK(s.frame == 0);
  rig.to_play();
  s = rig.session.state();
  CHECK(s.scene == "play");
  CHECK(s.paused);
  CHECK(s.frame == 1);
  CHECK_FALSE(s.ui.at("menu").shown);
  REQUIRE(s.entities.size() == 1);
  CHECK(s.entities[0].tags == std::vector<std::string>{"bird"});
  CHECK(s.entities[0].pos.y == doctest::Approx(20));
}

TEST_CASE("session: state carries a perf summary of the measured frames and the JS heap") {
  Rig rig;
  rig.to_play();
  rig.game.host.end_frame();
  const std::string perf = rig.session.state().perf;
  CHECK(perf.starts_with("{\"frames\":1,\"total\":{"));
  CHECK(perf.find("\"script\":{\"p50\":") != std::string::npos);
  CHECK(perf.find("\"jsBytes\":") != std::string::npos);
  CHECK(perf.find("\"jsObjects\":") != std::string::npos);
}

TEST_CASE("session: step needs a pause; clicks on hidden documents are refused") {
  Rig rig;
  CHECK(message_of([&] { rig.session.step(1); }).find("pause") != std::string::npos);
  rig.to_play();
  CHECK(message_of([&] { rig.session.click("menu", "start"); }).find("hidden") != std::string::npos);
  CHECK(message_of([&] { rig.session.click("nope", "x"); }).find("menu") != std::string::npos);
}

TEST_CASE("session: a tap lasts one step, a hold until released, per player") {
  Rig rig;
  rig.to_play();
  rig.session.action("jump", Press::Tap, 0);
  rig.session.step(2);
  CHECK(rig.game.logs() == "jump 1\ndown\n");
  rig.session.action("jump", Press::Hold, 1);
  rig.session.step(2);
  rig.session.action("jump", Press::Release, 1);
  rig.session.step(1);
  CHECK(rig.game.logs() == "jump 1\ndown\njump 2\ndown\ndown\n");
  CHECK(rig.session.state().frame == 6);
}

TEST_CASE("session: axes are held per player until released; unknown names list what exists") {
  Rig rig;
  rig.to_play();
  rig.session.axis("moveX", 1, -0.5f);
  rig.session.step(1);
  rig.session.axis("moveX", 1, std::nullopt);
  rig.session.step(1);
  CHECK(rig.game.logs() == "x -0.5\n");
  CHECK(message_of([&] { rig.session.action("fly", Press::Tap, 0); }).find("declares: jump") != std::string::npos);
  CHECK(message_of([&] { rig.session.axis("moveY", 0, 1.0f); }).find("declares: moveX") != std::string::npos);
}

TEST_CASE("session: screenshot is a PNG of the freshly drawn frame") {
  Rig rig;
  const auto png = rig.session.screenshot();
  REQUIRE(png.bytes.size() > 8);
  CHECK(png.bytes.substr(0, 4) == "\x89PNG");
  CHECK(png.width == 64);
  CHECK(png.height == 64);
}

TEST_CASE("session: fps averages recent frame times; quit is remembered; logs are the host's") {
  Rig rig;
  rig.session.frame_done(0.0);
  CHECK(rig.session.state().fps == 0.0);
  rig.session.frame_done(20.0);
  CHECK(rig.session.state().fps == doctest::Approx(50.0));
  rig.session.frame_done(10.0);
  CHECK(rig.session.state().fps == doctest::Approx(1000.0 * 2 / 30));
  for (int i = 0; i < 30; ++i) rig.session.frame_done(16.0);
  CHECK(rig.session.state().fps == doctest::Approx(62.5));
  CHECK_FALSE(rig.session.quit_requested());
  rig.session.quit();
  CHECK(rig.session.quit_requested());
  CHECK(&rig.session.log() == &rig.game.host.log());
}

#if ALPHA_DEBUG_TOOLS
TEST_CASE("session: reload rereads the game file and restarts the current scene") {
  Rig rig;
  rig.to_play();
  rig.game.box.write("game.js", "");
  CHECK(message_of([&] { rig.session.reload(); }).find("previous game keeps running") != std::string::npos);
  CHECK(rig.session.state().scene == "play");
  CHECK(rig.session.state().entities.size() == 1);
  rig.game.box.write("game.js",
                     "import { define } from 'alpha'; const { scene, game } = define();\n"
                     "const title = scene('title', {});\n"
                     "const play = scene('play', { enter(ctx) { console.log('v2'); } });\n"
                     "export default game({ config: { title: 't', size: [100, 100] }, start: 'title',\n"
                     "  scenes: { title, play } });\n");
  CHECK(message_of([&] { rig.session.reload(); }) == "");
  CHECK(rig.session.state().scene == "play");
  CHECK(rig.session.state().entities.empty());
  CHECK(rig.game.logs().find("v2") != std::string::npos);
  CHECK(rig.session.paused());  // the agent's pause survives a reload
}
#endif
