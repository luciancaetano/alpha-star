#include <doctest/doctest.h>

#include <filesystem>
#include <string>

#include <SDL3/SDL.h>

#include "host_fixture.hpp"
#include "script/host.hpp"

using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// A 640x360 game with a "move" stick (left half) and a "jump" button; logs
// moveX, jump pressed and the free touches each step. `touch` replaces the touch config.
std::string game(const std::string& touch = "touchSticks: { move: { zone: 'left' } },\n"
                                            "  touchButtons: { jump: { pos: [0.9, 0.8], label: 'A' } },",
                 const std::string& assets = "{}") {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { update(ctx) {\n"
         "  const i = ctx.input;\n"
         "  console.log(['step', i.axis('moveX').toFixed(2), i.pressed('jump'), i.touches().length].join(' '));\n"
         "} });\n"
         "export default game({ config: { title: 't', size: [640, 360],\n  " + touch + "\n"
         "  actions: { jump: ['Space', 'TouchJump'] },\n"
         "  axes: { moveX: ['A/D', 'TouchMoveX'] } },\n"
         "  start: 'play', scenes: { play }, assets: " + assets + " });\n";
}

std::string load_error(const Sandbox& box, const std::string& touch, const std::string& assets = "{}") {
  Host host(box.save());
  host.load(box.write("game.js", game(touch, assets)));
  host.step({}, kDt);
  return host.error();
}

std::string steps_logged(const Host& host) {
  std::string log;
  for (std::size_t i = 0; i < host.log().size(); ++i) {
    const std::string& line = host.log().at(i).text;
    if (line.rfind("step ", 0) == 0) log += line.substr(5) + "\n";
  }
  return log;
}

}  // namespace

TEST_CASE("simulate drives a touch stick and button end to end on a phone; desktop leaves them inert") {
  Sandbox box;
  const std::string g = std::filesystem::path(box.write("game.js", game())).generic_string();
  Host host(box.save());
  const auto results = host.test(box.write(
      "tests.js",
      "import { simulate } from 'alpha/test';\nimport game from '" + g + "';\n"
      "export function phone() {\n"
      "  simulate(game, { seconds: 4 / 60, platform: 'android', touches: [\n"
      "    { id: 1, path: [[0, [100, 200]], [1, [164, 200]], [3, null]] },\n"
      "    { id: 2, path: [[2, [576, 288]], [3, null]] }] }).expect.noErrors(); }\n"));
  REQUIRE(results.size() == 1);
  CHECK(results[0].passed());
  // Frame 0: stick grabbed at its center; 1: full right; 2: still right + jump; 3: both lifted.
  CHECK(steps_logged(host) == "0.00 false 0\n1.00 false 0\n1.00 true 0\n0.00 false 0\n");
}

TEST_CASE("on desktop the touch controls are inert: fingers are plain touches") {
  Sandbox box;
  const std::string g = std::filesystem::path(box.write("game.js", game())).generic_string();
  Host host(box.save());
  const auto results = host.test(box.write(
      "tests.js",
      "import { simulate } from 'alpha/test';\nimport game from '" + g + "';\n"
      "export function desktop() {\n"
      "  simulate(game, { seconds: 2 / 60, platform: 'windows',\n"
      "    touches: [{ id: 1, path: [[0, [100, 200]], [1, [164, 200]]] }] }).expect.noErrors(); }\n"
      "export function badPath() { simulate(game, { seconds: 0.1, touches: [{ id: 1, path: [[2, [0, 0]], [1, null]] }] }); }\n"));
  REQUIRE(results.size() == 2);
  CHECK(results[1].passed());
  CHECK(steps_logged(host) == "0.00 false 1\n0.00 false 1\n");
  CHECK(results[0].error.find("touches[0].path has [1,null]") != std::string::npos);
}

TEST_CASE("bad touch config fails at load naming the field and what it accepts") {
  Sandbox box;
  CHECK(load_error(box, "touchSticks: { move: { zone: 'top' } },")
            .find("config.touchSticks.move.zone: expected \"left\"|\"right\"|[x,y,w,h] in 0..1, got \"top\"") !=
        std::string::npos);
  CHECK(load_error(box, "touchSticks: { move: { deadzone: 1.2 } },")
            .find("config.touchSticks.move.deadzone must be in [0, 0.9], got 1.2") != std::string::npos);
  CHECK(load_error(box, "touchSticks: { mov: {} }, touchButtons: { jump: { pos: [0.9, 0.8] } },").find("touch stick sources: TouchMovX, TouchMovY") !=
        std::string::npos);
  CHECK(load_error(box, "touchSticks: { move: { skin: { base: 'ui/stick_base', knob: 'k' } } },")
            .find("config.touchSticks.move.skin.base: image \"ui/stick_base\" not in assets.images") !=
        std::string::npos);
  CHECK(load_error(box, "touchSticks: { move: { zone: [0, 0, 0.5, 1], axes: 'x', snap: 8 } }, touchButtons: { jump: { pos: [0.9, 0.8] } },").empty());
  CHECK(load_error(box, "touchSticks: { move: { zone: [0.8, 0, 0.5, 1] } },").find("got 0.8,0,0.5,1") !=
        std::string::npos);
  CHECK(load_error(box, "touchSticks: { move: { zone: 3 } },").find("[x,y,w,h] in 0..1, got 3") != std::string::npos);
  CHECK(load_error(box, "touchSticks: { move: { mode: 'sticky' } },")
            .find("config.touchSticks.move.mode: expected \"floating\"|\"fixed\", got \"sticky\"") != std::string::npos);
  CHECK(load_error(box, "touchSticks: { move: { scenes: ['menu'] } },")
            .find("config.touchSticks.move.scenes: \"menu\" is not a scene; scenes: play") != std::string::npos);
  CHECK(load_error(box, "touchSticks: { move: { radious: 3 } },").find("unknown field \"radious\"") !=
        std::string::npos);
  CHECK(load_error(box, "touchSticks: { move: {} }, touchButtons: { a: { pos: [0.5, 0.5] }, b: { pos: [0.52, 0.5] } },")
            .find("overlap") != std::string::npos);
}

TEST_CASE("skins resolve images and sheet frames; a near miss suggests the declared name") {
  Sandbox box;
  SDL_Surface* px = SDL_CreateSurface(16, 8, SDL_PIXELFORMAT_RGBA32);
  SDL_SaveBMP(px, (box.dir / "pad.bmp").string().c_str());
  SDL_DestroySurface(px);
  const std::string assets = "{ images: { 'ui/stick_knob': 'pad.bmp' }, sheets: { pad: { image: 'pad.bmp', frame: [8, 8] } } }";
  const std::string skinned =
      "touchSticks: { move: { mode: 'fixed', skin: { base: 'pad:0', knob: 'ui/stick_knob', pressed: { knob: 'pad:1' },"
      " tint: '#ffd27fcc' } } },\n  touchButtons: { jump: { pos: [0.9, 0.8], skin: { up: 'pad:0', down: 'pad:1' } } },";
  CHECK(load_error(box, skinned, assets).empty());
  std::string typo = skinned;
  typo.replace(typo.find("knob: 'ui/stick_knob'"), 21, "knob: 'ui/stick_knb'");
  CHECK(load_error(box, typo, assets).find("image \"ui/stick_knb\" not in assets.images; did you mean \"ui/stick_knob\"?") !=
        std::string::npos);
  std::string frame = skinned;
  frame.replace(frame.find("'pad:1' }, tint"), 7, "'pad:7'");
  CHECK(load_error(box, frame, assets).find("sheet \"pad\" has frames 0..1") != std::string::npos);
}
