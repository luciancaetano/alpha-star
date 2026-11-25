#include <doctest/doctest.h>

#include <filesystem>
#include <string>

#include "host_fixture.hpp"
#include "script/host.hpp"

using alpha::core::InputFrame;
using alpha::core::parse_input;
using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// Logs what the input API reports each step; `extra` runs after it.
std::string game(const std::string& extra = "") {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { update(ctx) {\n"
         "  const i = ctx.input;\n"
         "  console.log([i.axis('moveX').toFixed(2), i.player(2).axis('moveX').toFixed(2),\n"
         "    i.pressed('jump'), i.player(1).pressed('jump'), i.player(2).down('jump'), i.players()].join(' '));\n"
         "  " + extra + " } });\n"
         "export default game({ config: { title: 't', size: [100, 100], deadzone: 0.25,\n"
         "  actions: { jump: ['GamepadA', 'GamepadLeftY-'] },\n"
         "  axes: { moveX: ['GamepadLeftX', 'A/D'], throttle: ['GamepadRT'] } },\n"
         "  start: 'play', scenes: { play } });\n";
}

std::string last_line(const Host& host) { return host.log().at(host.log().size() - 1).text; }

std::string step_line(Host& host, const InputFrame& frame) {
  host.step(frame, kDt);
  REQUIRE_MESSAGE(!host.paused(), host.error());
  return last_line(host);
}

std::string error_of(const Sandbox& box, const std::string& extra) {
  Host host(box.save());
  host.load(box.write("game.js", game(extra)));
  host.step({}, kDt);
  return host.error();
}

}  // namespace

TEST_CASE("ctx.input reads axes after the deadzone, per player and for any player") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("game.js", game()));
  InputFrame f;
  f.players[0].held.set(*parse_input("D"));
  f.players[1].analog[0] = -0.625f;  // (0.625 - 0.25) / 0.75 = 0.5
  f.players[1].held.set(*parse_input("GamepadA"));
  f.connected = 2;
  CHECK(step_line(host, f) == "1.00 -0.50 true false true 2");
  InputFrame stick_up;
  stick_up.players[0].analog[1] = -0.9f;  // digital view: GamepadLeftY- presses jump
  CHECK(step_line(host, stick_up) == "0.00 0.00 true true false 1");
}

TEST_CASE("rumble requests reach the host once; bad calls name what is allowed") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("game.js", game("if (ctx.input.pressed('jump')) ctx.input.rumble(0.5, 0.2, 2);")));
  InputFrame f;
  f.players[0].held.set(*parse_input("GamepadA"));
  host.step(f, kDt);
  const auto r = host.take_rumble();
  CHECK_FALSE(r[0]);
  REQUIRE(r[1]);
  CHECK(r[1]->strength == doctest::Approx(0.5f));
  CHECK_FALSE(host.take_rumble()[1]);

  CHECK(error_of(box, "ctx.input.rumble(2, 1);").find("rumble strength must be 0..1") != std::string::npos);
  CHECK(error_of(box, "ctx.input.rumble(1, 1, 5);").find("player must be an integer 1..4, got 5") !=
        std::string::npos);
  CHECK(error_of(box, "ctx.input.player(0);").find("input.player(0): player must be") != std::string::npos);
  const std::string unknown = error_of(box, "ctx.input.axis('moveY');");
  CHECK(unknown.find("unknown axis \"moveY\"; config.axes declares: moveX throttle") != std::string::npos);
}

TEST_CASE("bad axis config and the reserved action name fail at load with a reason") {
  Sandbox box;
  Host host(box.save());
  std::string bad = game();
  bad.replace(bad.find("'A/D'"), 5, "'A/Q/D'");
  host.load(box.write("game.js", bad));
  CHECK(host.error().find("axis \"moveX\": unknown source \"A/Q/D\"") != std::string::npos);
  std::string reserved = game();
  reserved.replace(reserved.find("jump: ["), 4, "axes");
  Host host2(box.save());
  host2.load(box.write("game.js", reserved));
  CHECK(host2.error().find("\"axes\" is reserved") != std::string::npos);
}

TEST_CASE("simulate holds analog values until changed and drives each player") {
  Sandbox box;
  const std::string g = std::filesystem::path(box.write("game.js", game())).generic_string();
  Host host(box.save());
  const auto results = host.test(box.write(
      "tests.js",
      "import { simulate } from 'alpha/test';\nimport game from '" + g + "';\n"
      "export function axes() {\n"
      "  const r = simulate(game, { seconds: 0.1, inputs: [{ axes: { moveX: [[0, 1], [3, -0.5]] } },\n"
      "    { jump: [2], axes: { moveX: [[1, 0.25]] } }] });\n"
      "  r.expect.noErrors(); }\n"
      "export function badAxis() { simulate(game, { seconds: 0.1, inputs: { axes: { moveY: [[0, 1]] } } }); }\n"
      "export function badPoint() { simulate(game, { seconds: 0.1, inputs: { axes: { moveX: [[5, 1], [2, 0]] } } }); }\n"));
  REQUIRE(results.size() == 3);
  CHECK(results[0].passed());
  CHECK(results[1].error.find("inputs.axes.moveY is not an axis; axes: moveX, throttle") != std::string::npos);
  CHECK(results[2].error.find("inputs.axes.moveX has [2,0]") != std::string::npos);
  std::string log;
  for (std::size_t i = 0; i < host.log().size(); ++i) log += host.log().at(i).text + "\n";
  CHECK(log.find("1.00 0.00 false false false 2\n1.00 0.25 false false false 2\n"
                 "1.00 0.25 true false true 2\n-0.50 0.25 false false false 2\n") != std::string::npos);
}
