#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::Sandbox;
using alpha::test::ui_text;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// title: "Start" button -> play; play: score counts steps, shows the hud;
// pressing jump (Space / gamepad A) counts in the hud.
constexpr const char* kGame = R"(
import { define } from 'alpha'; const { scene, game } = define();
const title = scene('title', {
  enter(ctx) {
    ctx.ui.show('menu');
    ctx.ui.on('menu', 'start', 'click', () => ctx.go('play'));
  },
});
const play = scene('play', {
  state: () => ({ steps: 0, jumps: 0 }),
  enter(ctx) { ctx.ui.show('hud'); },
  update(ctx) {
    ctx.state.steps += 1;
    if (ctx.input.pressed('jump')) ctx.state.jumps += 1;
    ctx.ui.set('hud', 'score', ctx.state.jumps);
  },
});
export default game({ config: { title: 't', size: [100, 100], seed: 1,
    actions: { jump: ['Space', 'GamepadA', 'MouseLeft'] } },
  start: 'title', scenes: { title, play },
  assets: { sounds: {}, ui: { menu: 'menu.rml', hud: 'hud.rml' } } });
)";

constexpr const char* kMenu = R"(<rml><head><style>
body { width: 100%; height: 100%; }
button { display: block; width: 50px; height: 20px; tab-index: auto; }
</style></head><body><button id="start"></button></body></rml>)";

std::string write_game(const Sandbox& box) {
  box.write("menu.rml", kMenu);
  box.write("hud.rml", alpha::test::rml({"score"}));
  return box.write("game.js", kGame);
}

SDL_Event pad(SDL_GamepadButton button, bool down) {
  SDL_Event e{};
  e.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
  e.gbutton.button = static_cast<Uint8>(button);
  e.gbutton.down = down;
  return e;
}

// Down and up; true if the down reached the game.
bool press(Host& host, SDL_GamepadButton button) {
  const bool reached = host.reaches_game(pad(button, true));
  host.reaches_game(pad(button, false));
  return reached;
}

}  // namespace

TEST_CASE("script ui: a click runs its handler on the next step; scene change hides and drops handlers") {
  Sandbox box;
  Host host(box.save());
  host.load(write_game(box));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(host.ui()->view().at("menu").shown);
  host.ui()->click("menu", "start");
  CHECK(host.scene() == "title");  // handlers run inside steps only
  host.step({}, kDt);
  CHECK(host.scene() == "play");
  CHECK_FALSE(host.ui()->view().at("menu").shown);
  CHECK(host.ui()->view().at("hud").shown);
  host.step({}, kDt);
  CHECK(ui_text(host, "score") == "0");
}

TEST_CASE("script ui: a gamepad A the UI consumed never reaches game actions") {
  Sandbox box;
  Host host(box.save());
  host.load(write_game(box));
  CHECK(press(host, SDL_GAMEPAD_BUTTON_SOUTH));  // nothing focused yet: A is the game's
  CHECK_FALSE(press(host, SDL_GAMEPAD_BUTTON_DPAD_DOWN));  // focuses Start
  CHECK_FALSE(press(host, SDL_GAMEPAD_BUTTON_SOUTH));  // clicks it, not the game
  host.step({}, kDt);
  CHECK(host.scene() == "play");
  CHECK(press(host, SDL_GAMEPAD_BUTTON_SOUTH));  // the hud has nothing focusable
}

TEST_CASE("script ui: mistakes name the call and what exists") {
  Sandbox box;
  box.write("hud.rml", alpha::test::rml({"score", "best"}));
  const auto error_of = [&](const std::string& body) {
    Host host(box.save());
    std::string game = alpha::test::tiny("enter", body);
    const std::string assets = "assets: { sounds: {} }";
    game.replace(game.find(assets), assets.size(), "assets: { sounds: {}, ui: { hud: 'hud.rml' } }");
    host.load(box.write("g.js", game));
    return host.error();
  };
  CHECK(error_of("ctx.ui.show('menu');").find("ui.show(\"menu\"): unknown document \"menu\"; assets.ui has: hud") !=
        std::string::npos);
  CHECK(error_of("ctx.ui.set('hud', 'scroe', 1);").find("ui.set(\"hud\", \"scroe\"): no element with id") !=
        std::string::npos);
  CHECK(error_of("ctx.ui.set('hud', 'scroe', 1);").find("ids: score, best") != std::string::npos);
  CHECK(error_of("ctx.ui.on('hud', 'score', 'click');").find("fn must be a function") != std::string::npos);
  CHECK(error_of("ctx.ui.on('hud', 'score', 'tap', () => {});").find("events: click, back") != std::string::npos);
}

TEST_CASE("script ui: a missing document or font stops boot with its name") {
  Sandbox box;
  const auto boot_error = [&](const std::string& assets) {
    Host host(box.save());
    std::string game = alpha::test::tiny("enter", "");
    const std::string empty = "assets: { sounds: {} }";
    game.replace(game.find(empty), empty.size(), "assets: { sounds: {}, " + assets + " }");
    host.load(box.write("g.js", game));
    return host.error();
  };
  CHECK(boot_error("ui: { hud: 'nope.rml' }").find("nope.rml") != std::string::npos);
  CHECK(boot_error("fonts: { pixel: 'nope.ttf' }").find("cannot open font file") != std::string::npos);
  box.write("bad.ttf", "not a font");
  CHECK(boot_error("fonts: { pixel: 'bad.ttf' }").find("is not a font") != std::string::npos);
}

TEST_CASE("simulate: clicks by document#id and text expectations") {
  Sandbox box;
  const std::string game = std::filesystem::path(write_game(box)).generic_string();
  Host host(box.save());
  const auto results = host.test(box.write("t.js", "import { simulate } from 'alpha/test';\n"
      "import game from '" + game + "';\n"
      "export function starts() { const r = simulate(game, { seconds: 1, clicks: { 'menu#start': [10] },"
      " inputs: { jump: [20, 30] } }); r.expect.scene('play'); r.expect.shown('hud');"
      " r.expect.text('hud', 'score').equals('2'); }\n"
      "export function wrongText() { simulate(game, { seconds: 0.5, clicks: { 'menu#start': [0] } })"
      ".expect.text('hud', 'score').equals('9'); }\n"
      "export function unknownId() { simulate(game, { seconds: 0.5 }).expect.text('hud', 'scor').equals(''); }\n"
      "export function hiddenClick() { simulate(game, { seconds: 0.5, clicks: { 'hud#score': [3] } }); }\n"));
  REQUIRE(results.size() == 4);
  CHECK(results[0].error.find("clicks[\"hud#score\"] at frame 3: document \"hud\" is hidden") != std::string::npos);
  CHECK_MESSAGE(results[1].passed(), results[1].error);
  CHECK(results[2].error.find("expect.text(\"hud\", \"scor\"): no element with that id; ids: score") !=
        std::string::npos);
  CHECK(results[3].error.find("expected hud#score text \"9\" after 30 frames, got \"0\"") != std::string::npos);
}

TEST_CASE("script ui: drawn over the game; release frees its textures") {
  Sandbox box;
  box.write("menu.rml", "<rml><head><style>body { width: 100%; height: 100%; } #start { width: 100px; height: 100px; "
            "background-color: #ff0000; }</style></head><body><div id=\"start\"></div></body></rml>");
  box.write("hud.rml", alpha::test::rml({"score"}));
  Host host(box.save());
  host.load(box.write("game.js", kGame));
  SDL_Surface* surface = SDL_CreateSurface(100, 100, SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* sdl = SDL_CreateSoftwareRenderer(surface);
  const auto red_at_center = [&] {
    SDL_SetRenderDrawColor(sdl, 0, 0, 0, 255);
    SDL_RenderClear(sdl);
    host.draw_ui(sdl, 0.0);
    SDL_RenderPresent(sdl);
    Uint8 r = 0, g = 0, b = 0, a = 0;
    SDL_ReadSurfacePixel(surface, 50, 50, &r, &g, &b, &a);
    return r == 255;
  };
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(host.ui()->view().at("menu").shown);
  CHECK(red_at_center());
  host.ui()->click("menu", "start");
  host.step({}, kDt);
  CHECK(host.scene() == "play");
  host.release_ui();
  SDL_DestroyRenderer(sdl);
  SDL_DestroySurface(surface);
}
