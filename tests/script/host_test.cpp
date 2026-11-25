#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <SDL3/SDL.h>

#include "script/host.hpp"

using alpha::core::InputSet;
using alpha::core::parse_input;
using alpha::script::Host;
namespace fs = std::filesystem;

namespace {

constexpr float kDt = 1.0f / 60.0f;

const char* kGame = R"js(
import { define } from "alpha";
const { scene, game } = define();

const play = scene("play", {
  state: () => ({ ticks: 0, flaps: 0 }),
  enter(ctx) {
    ctx.spawn({ tags: ["ground"], pos: [50, 95], shape: { rect: [100, 10], color: "#00ff00" },
                body: { type: "static", collider: { rect: [100, 10] } } });
    ctx.spawn({ tags: ["ball"], pos: [50, 10], shape: { circle: 5, color: "#ff0000", outline: "#000" },
                body: { type: "dynamic", collider: { circle: 5 } } });
    ctx.ui.text(() => `flaps ${ctx.state.flaps} coins ${ctx.findAll("coin").length}`,
                { pos: [50, 5], size: 8, align: "center" });
    ctx.onHit("ball", "ground", () => ctx.go("over", { ticks: ctx.state.ticks }));
    ctx.every(0.25, () => ctx.spawn({ tags: ["coin"], pos: [10, 10] }));
  },
  update(ctx) {
    ctx.state.ticks += 1;
    if (ctx.input.pressed("flap")) {
      ctx.state.flaps += 1;
      ctx.find("ball").body.velocity = [0, -60];
    }
  },
});

const over = scene("over", {
  enter(ctx, { ticks }) {
    ctx.ui.text(`over after ${ticks} best ${ctx.save.get("best", -1)}`, { pos: [0, 50] });
    ctx.save.set("best", ticks);
  },
});

export default game({
  config: { title: "test", size: [100, 100], background: "#102030", seed: 1,
            gravity: [0, 900], actions: { flap: ["Space"] } },
  start: "play",
  scenes: { play, over },
  assets: { sounds: {} },
});
)js";

struct Sandbox {
  fs::path dir = fs::temp_directory_path() / "alpha_host_test";
  Sandbox() {
    fs::remove_all(dir);
    fs::create_directories(dir);
  }
  ~Sandbox() { fs::remove_all(dir); }
  std::string write(const std::string& name, const std::string& source) const {
    std::ofstream(dir / name) << source;
    return (dir / name).string();
  }
  std::string save() const { return (dir / "save.json").string(); }
};

InputSet space() { return InputSet{}.set(*parse_input("Space")); }

std::string first_text(Host& host) {
  const auto texts = host.frame().texts;
  return texts.empty() ? "" : texts.front().text;
}

// A game whose `play` scene runs `body` in update (or enter).
std::string tiny(const std::string& hook, const std::string& body) {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { " + hook + "(ctx) { " + body + " } });\n"
         "export default game({ config: { title: 't', size: [288, 100], seed: 1,\n"
         "  actions: { jump: ['Space'] } }, start: 'play', scenes: { play },\n"
         "  assets: { sounds: {} } });\n";
}

}  // namespace

TEST_CASE("host boots a JS game: config, shapes and live text") {
  Sandbox box;
  Host host(box.save());
  const auto info = host.load(box.write("game.js", kGame));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(info.title == "test");
  CHECK(info.width == 100);
  CHECK(info.background == alpha::core::Color{0x10, 0x20, 0x30, 255});
  const auto frame = host.frame();
  CHECK(frame.shapes.size() == 2);
  REQUIRE(frame.texts.size() == 1);
  CHECK(frame.texts[0].text == "flaps 0 coins 0");
  CHECK(frame.texts[0].align == alpha::core::Align::Center);
}

TEST_CASE("input actions, timers and velocity reach the script") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("game.js", kGame));
  host.step(space(), kDt);
  host.step(space(), kDt);  // held: not pressed again
  for (int i = 0; i < 14; ++i) host.step({}, kDt);
  CHECK(first_text(host) == "flaps 1 coins 1");
}

TEST_CASE("collision handler switches scene with params; save persists across runs") {
  Sandbox box;
  const std::string path = box.write("game.js", kGame);
  {
    Host host(box.save());
    host.load(path);
    for (int i = 0; i < 120 && first_text(host).rfind("over", 0) != 0; ++i) host.step({}, kDt);
    CHECK(first_text(host).rfind("over after ", 0) == 0);
    CHECK(first_text(host).find("best -1") != std::string::npos);
    CHECK(host.frame().shapes.empty());  // new scene, new world
  }
  Host again(box.save());
  again.load(path);
  for (int i = 0; i < 120 && first_text(again).rfind("over", 0) != 0; ++i) again.step({}, kDt);
  CHECK(first_text(again).find("best -1") == std::string::npos);
}

TEST_CASE("entity proxies read and write the engine world") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("e.js", tiny("enter", R"(
    const e = ctx.spawn({ tags: ['a', 'b'], pos: [1, 2], rotation: 5, z: 3 });
    const check = (ok, what) => { if (!ok) throw new Error('failed: ' + what); };
    check(e.tags.join() === 'a,b', 'tags');
    check(e.pos.x === 1 && e.pos.y === 2 && e.rotation === 5 && e.z === 3, 'spec read back');
    e.pos.x = 10; e.pos.y = 20; e.rotation = 45; e.z = 7;
    check(e.pos.x === 10 && e.pos.y === 20 && e.rotation === 45 && e.z === 7, 'writes');
    check(ctx.find('b') === e && ctx.random.pick([4]) === 4, 'identity + pick');
    check(ctx.random.int(2, 2) === 2 && ctx.random.range(1, 1) === 1, 'random');
    ctx.after(0.5, () => {});
    ctx.destroy(e);
    check(!e.alive && ctx.findAll('a').length === 0, 'destroy');
  )")));
  CHECK(host.error() == "");
}

TEST_CASE("assets preload at boot; audio plays files, synth assets and tones") {
  Sandbox box;
  SDL_Surface* px = SDL_CreateSurface(2, 2, SDL_PIXELFORMAT_RGBA32);
  SDL_SaveBMP(px, (box.dir / "hero.bmp").string().c_str());
  SDL_DestroySurface(px);
  const std::string game = R"js(
import { define } from "alpha";
const { scene, game } = define();
const play = scene("play", {
  enter(ctx) {
    ctx.spawn({ tags: ["hero"], pos: [10, 10], shape: { image: "hero" } });
    ctx.spawn({ tags: ["hero"], pos: [30, 10], shape: { image: "hero", size: [8, 4] } });
    const v = ctx.audio.play("beep", { volume: 0.5, pitch: 2, loop: true });
    ctx.audio.tone([{ wave: "square", freq: [440, 880], duration: 0.05 }, { wave: "noise", freq: 1, duration: 0.05 }]);
    ctx.audio.volume = 0.25;
    if (!v.playing || ctx.audio.volume !== 0.25) throw new Error("voice not playing");
    v.stop();
    if (v.playing) throw new Error("voice did not stop");
  },
});
export default game({ config: { title: "a", size: [288, 100], seed: 1, actions: {} }, start: "play",
  scenes: { play },
  assets: { sounds: { beep: { wave: "sine", freq: 440, duration: 0.1, volume: 0.3 } },
            images: { hero: "hero.bmp" } } });
)js";
  Host host(box.save());
  host.load(box.write("a.js", game));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(host.mixer().voices() == 1);  // the tone; the looped voice was stopped
  CHECK(host.mixer().volume() == 0.25f);
  const auto shapes = host.frame().shapes;
  REQUIRE(shapes.size() == 2);
  REQUIRE(shapes[0].shape.image);
  CHECK(std::get<alpha::core::Rect>(shapes[0].shape.geometry).size == alpha::core::Vec2{2, 2});
  CHECK(std::get<alpha::core::Rect>(shapes[1].shape.geometry).size == alpha::core::Vec2{8, 4});
}

TEST_CASE("asset and audio mistakes name the asset and what exists") {
  Sandbox box;
  const auto boot_error = [&](const std::string& assets, const std::string& body) {
    Host host(box.save());
    host.load(box.write("g.js",
        "import { define } from 'alpha'; const { scene, game } = define();\n"
        "const play = scene('play', { enter(ctx) { " + body + " } });\n"
        "export default game({ config: { title: 't', size: [288, 100], seed: 1, actions: {} },\n"
        "  start: 'play', scenes: { play }, assets: " + assets + " });\n"));
    return host.error();
  };
  CHECK(boot_error("{ sounds: { s: 'missing.wav' } }", "").find("missing.wav") != std::string::npos);
  CHECK(boot_error("{ images: { i: 'missing.png' } }", "").find("PNG, JPG or BMP") != std::string::npos);
  CHECK(boot_error("{ sounds: { s: { wave: 'organ', freq: 1, duration: 1 } } }", "")
            .find("assets.sounds.s.wave must be sine, square") != std::string::npos);
  CHECK(boot_error("{ sounds: { s: [] } }", "").find("at least one note") != std::string::npos);
  CHECK(boot_error("{ sounds: { a: { wave: 'sine', freq: 99999, duration: 1 } } }", "")
            .find("freq must be in") != std::string::npos);
  CHECK(boot_error("{ sounds: { a: { wave: 'sine', freq: 9, duration: 1 } } }", "ctx.audio.play('b');")
            .find("assets.sounds has: a") != std::string::npos);
  CHECK(boot_error("{}", "ctx.spawn({ tags: [], pos: [0, 0], shape: { image: 'x' } });")
            .find("not in assets.images; have: (none)") != std::string::npos);
  CHECK(boot_error("{}", "ctx.audio.volume = 2;").find("audio.volume must be in [0, 1]") !=
        std::string::npos);
  CHECK(boot_error("{}", "ctx.audio.tone({ wave: 'sine', freq: [1, 2, 3], duration: 1 });")
            .find("audio.tone.freq must be [x, y]") != std::string::npos);
}

TEST_CASE("a script error silences the game") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("s.js", tiny("update", "ctx.audio.tone({ wave: 'sine', freq: 440, duration: 5 }); throw new Error('x');")));
  host.step({}, kDt);
  CHECK(host.paused());
  CHECK(host.mixer().voices() == 0);
}

TEST_CASE("a throw in update pauses the game and shows where") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("boom.js", tiny("update", "throw new Error('boom');")));
  REQUIRE_FALSE(host.paused());
  host.step({}, kDt);
  REQUIRE(host.paused());
  CHECK(host.error().find("boom") != std::string::npos);
  CHECK(host.error().find("boom.js") != std::string::npos);
  host.step({}, kDt);  // stays paused, no crash
  CHECK(first_text(host) == "SCRIPT ERROR (game paused)");
}

TEST_CASE("script mistakes produce errors written for the agent") {
  Sandbox box;
  const auto error_of = [&](const std::string& hook, const std::string& body) {
    Host host(box.save());
    host.load(box.write("g.js", tiny(hook, body)));
    host.step({}, kDt);
    return host.error();
  };
  CHECK(error_of("enter", "ctx.spawn({ tags: [], pos: [0, 0], shape: { circle: 3, color: 'red' } });")
            .find("shape.color must be a color like") != std::string::npos);
  CHECK(error_of("enter", "ctx.spawn({ tags: [], pos: [0] });").find("spawn: pos must be [x, y]") !=
        std::string::npos);
  CHECK(error_of("update", "Math.random();").find("use ctx.random") != std::string::npos);
  CHECK(error_of("update", "ctx.input.pressed('fly');").find("unknown action \"fly\"") !=
        std::string::npos);
  CHECK(error_of("update", "ctx.go('nowhere');").find("scenes: play") != std::string::npos);
  CHECK(error_of("update", "ctx.find('ghost');").find("no living entity") != std::string::npos);
  CHECK(error_of("update", "ctx.audio.play('beep');").find("unknown sound") != std::string::npos);
}

TEST_CASE("load failures pause with a readable reason") {
  Sandbox box;
  const auto load_error = [&](const std::string& source) {
    Host host(box.save());
    if (source.empty()) {
      host.load((box.dir / "missing.js").string());
    } else {
      host.load(box.write("bad.js", source));
    }
    return host.error();
  };
  CHECK(load_error("").find("cannot open module") != std::string::npos);
  CHECK(load_error("export const x = 1;").find("export default game") != std::string::npos);
  CHECK(load_error("export default {").find("SyntaxError") != std::string::npos);
}
