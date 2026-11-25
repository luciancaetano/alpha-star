#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "host_fixture.hpp"
#include "platform/save_file.hpp"

using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

namespace fs = std::filesystem;

// A game whose play scene runs `enter`; `config` and `assets` are appended.
std::string game(const std::string& config, const std::string& assets, const std::string& enter) {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const check = (ok, what) => { if (!ok) throw new Error('check failed: ' + what); };\n"
         "const play = scene('play', { enter(ctx) {\n" + enter + "\n} });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 1, actions: { jump: ['Space'] }" + config +
         " }, start: 'play', scenes: { play }, assets: {" + assets + "} });\n";
}

// Runs a game once (a fresh engine run) and returns its error ("" when none).
std::string run(const Sandbox& box, const std::string& source) {
  Host host(box.save());
  host.load(box.write("g.js", source));
  return host.error();
}

std::string file(const Sandbox& box, const std::string& name) {
  return alpha::platform::read_file(box.dir / name).value_or("(missing)");
}

}  // namespace

TEST_CASE("save slots are versioned files; an older version migrates once") {
  Sandbox box;
  REQUIRE(run(box, game("", "", "ctx.save.set('coins', 3); ctx.save.slot('b').set('lvl', 2);")) == "");
  CHECK(file(box, "save.json") == R"({"version":1,"data":{"coins":3}})");
  CHECK(file(box, "save-b.json") == R"({"version":1,"data":{"lvl":2}})");
  const std::string v2 = ", save: { version: 2, migrate(d, from) { check(from === 1, 'from'); return { gold: d.coins ?? 0, lvl: d.lvl ?? 0 }; } }";
  REQUIRE(run(box, game(v2, "", "check(ctx.save.get('gold') === 3, 'gold'); check(ctx.save.slot('b').get('lvl') === 2, 'lvl');"
                                "check(!ctx.save.has('coins'), 'old key gone'); check(ctx.save.keys().join() === 'gold,lvl', 'keys');")) == "");
  CHECK(file(box, "save.json") == R"({"version":2,"data":{"gold":3,"lvl":0}})");
  CHECK(run(box, game(", save: { version: 3 }", "", "ctx.save.get('gold');")).find("add config.save.migrate(data, fromVersion)") !=
        std::string::npos);
  CHECK(run(box, game("", "", "ctx.save.slot('a b');")).find("slot names are 1..32 letters") != std::string::npos);
  CHECK(run(box, game("", "", "ctx.save.set('f', () => 1);")).find("value must be JSON-safe") != std::string::npos);
}

TEST_CASE("a save from before versioning is version 1; a corrupt save falls back to its .bak") {
  Sandbox box;
  std::ofstream(box.dir / "save.json") << R"({"best": 9})";
  REQUIRE(run(box, game("", "", "check(ctx.save.get('best') === 9, 'legacy'); ctx.save.set('best', 10);")) == "");
  CHECK(file(box, "save.json.bak") == R"({"best": 9})");
  std::ofstream(box.dir / "save.json", std::ios::trunc) << "{\"vers";  // a crash mid-write elsewhere
  REQUIRE(run(box, game("", "", "check(ctx.save.get('best') === 9, 'from backup');")) == "");
  fs::remove(box.dir / "save.json.bak");
  std::ofstream(box.dir / "save.json", std::ios::trunc) << "garbage";
  REQUIRE(run(box, game("", "", "check(ctx.save.get('best', 'none') === 'none', 'empty');")) == "");
}

TEST_CASE("settings persist separately and apply at boot: volumes, locale, bindings, accessibility") {
  Sandbox box;
  box.write("en.json", R"({ "hi": "Hello" })");
  box.write("pt.json", R"({ "hi": "Ola" })");
  const std::string assets = "locales: { en: 'en.json', pt: 'pt.json' }";
  REQUIRE(run(box, game("", assets,
                        "ctx.audio.bus('music').volume = 0.5; ctx.locale = 'pt'; ctx.input.rebind('jump', ['W', 'GamepadA']);"
                        "ctx.settings.uiScale = 1.5; ctx.settings.highContrast = true; ctx.settings.reducedMotion = true;")) == "");
  CHECK(file(box, "settings.json").find(R"("locale":"pt")") != std::string::npos);
  CHECK(file(box, "save.json") == "(missing)");
  Host host(box.save());
  host.load(box.write("g.js", game("", assets,
      "check(ctx.locale === 'pt' && ctx.t('hi') === 'Ola', 'locale');"
      "check(ctx.settings.volumes.music === 0.5 && ctx.settings.volumes.sfx === 1, 'volumes');"
      "check(ctx.input.bindings('jump').join() === 'W,GamepadA', 'bindings');"
      "check(ctx.settings.uiScale === 1.5 && ctx.settings.highContrast && ctx.settings.reducedMotion, 'flags');")));
  REQUIRE_MESSAGE(host.error() == "", host.error());
  CHECK(host.mixer().bus_volume(alpha::core::Mixer::kMusic) == 0.5f);
  CHECK(run(box, game("", assets, "ctx.settings.reset(); check(ctx.locale === 'en' && ctx.settings.uiScale === 1, 'reset');"
                                  "check(ctx.input.bindings('jump').join() === 'Space', 'bindings reset');")) == "");
  CHECK(run(box, game("", assets, "ctx.settings.uiScale = 9;")).find("settings.uiScale must be a number in [0.5, 3]") != std::string::npos);
  CHECK(run(box, game("", assets, "ctx.input.rebind('fly', ['W']);")).find("input.rebind(\"fly\"): not in config.actions; actions: jump") !=
        std::string::npos);
  CHECK(run(box, game("", assets, "ctx.locale = 'fr';")).find("\"fr\" is not in assets.locales; locales: en, pt") != std::string::npos);
}

TEST_CASE("unusable settings from the file are skipped, never fatal") {
  Sandbox box;
  std::ofstream(box.dir / "settings.json") << R"({"volumes":{"gone":0.3,"sfx":7},"locale":"xx","bindings":{"fly":["W"]},"uiScale":"big"})";
  CHECK(run(box, game("", "", "check(ctx.settings.uiScale === 1, 'scale default');")) == "");
}

TEST_CASE("t() interpolates, picks plural forms, falls back to config.locale, shows [key] when missing") {
  Sandbox box;
  box.write("en.json", R"({ "score": "Score: {n}", "menu": { "play": "Play" }, "coins": { "zero": "no coins", "one": "{count} coin", "other": "{count} coins" }, "only": "en only" })");
  box.write("pt.json", R"({ "score": "Pontos: {n}", "menu": { "play": "Jogar" }, "coins": { "one": "{count} moeda", "other": "{count} moedas" } })");
  const std::string assets = "locales: { en: 'en.json', pt: 'pt.json' }";
  CHECK(run(box, game(", locale: 'en'", assets,
      "check(ctx.t('score', { n: 5 }) === 'Score: 5', 'fill');"
      "check(ctx.t('menu.play') === 'Play', 'nested');"
      "check(ctx.t('coins', { count: 0 }) === 'no coins' && ctx.t('coins', { count: 1 }) === '1 coin' && ctx.t('coins', { count: 7 }) === '7 coins', 'plural');"
      "ctx.locale = 'pt';"
      "check(ctx.t('coins', { count: 0 }) === '0 moedas', 'no zero form');"
      "check(ctx.t('only') === 'en only', 'fallback');"
      "check(ctx.t('nope') === '[nope]', 'missing');")) == "");
  box.write("bad.json", "{ nope");
  CHECK(run(box, game("", "locales: { en: 'bad.json' }", "")).find("assets.locales.en (\"bad.json\") is not valid JSON") != std::string::npos);
}

TEST_CASE("RmlUi elements with data-t show translations, on boot and when the locale changes") {
  Sandbox box;
  box.write("en.json", R"({ "title": "Settings", "volume": "Volume" })");
  box.write("pt.json", R"({ "title": "Ajustes", "volume": "Volume" })");
  box.write("menu.rml", R"(<rml><head><style>body { width: 100%; }</style></head><body>)"
                        R"(<h1 id="t" data-t="title">x</h1><p id="v" data-t="volume"></p></body></rml>)");
  Host host(box.save());
  host.load(box.write("g.js", game("", "locales: { en: 'en.json', pt: 'pt.json' }, ui: { menu: 'menu.rml' }",
                                   "ctx.ui.show('menu'); ctx.after(0.05, () => { ctx.locale = 'pt'; });")));
  REQUIRE_MESSAGE(host.error() == "", host.error());
  CHECK(alpha::test::ui_text(host, "t") == "Settings");
  for (int i = 0; i < 6; ++i) host.step({}, 1.0f / 60.0f);
  CHECK(alpha::test::ui_text(host, "t") == "Ajustes");
  CHECK(alpha::test::ui_text(host, "v") == "Volume");
}

TEST_CASE("music streams from assets.music: fades, crossfades, stops; one track at a time") {
  Sandbox box;
  fs::copy_file(std::string(ALPHA_TEST_FIXTURES) + "/audio/tone.ogg", box.dir / "a.ogg", fs::copy_options::overwrite_existing);
  fs::copy_file(std::string(ALPHA_TEST_FIXTURES) + "/audio/tone.mp3", box.dir / "b.mp3", fs::copy_options::overwrite_existing);
  const std::string assets = "music: { a: 'a.ogg', b: 'b.mp3' }, sounds: { v: { synth: { wave: 'sine', freq: 440, duration: 0.1 }, bus: 'voice' } }";
  Host host(box.save());
  host.load(box.write("g.js", game(", audio: { buses: ['crowd'] }", assets,
      "check(ctx.audio.musicPlaying === null, 'silent');"
      "ctx.audio.music('a', { fadeIn: 1 }); check(ctx.audio.musicPlaying === 'a', 'a');"
      "ctx.audio.crossfade('b', 0.5, { volume: 0.8 }); check(ctx.audio.musicPlaying === 'b', 'b');"
      "ctx.audio.play('v', { pan: -1 }); check(ctx.audio.buses.join() === 'master,music,sfx,voice,crowd', 'buses');")));
  REQUIRE_MESSAGE(host.error() == "", host.error());
  CHECK(host.mixer().music_name() == "b");
  CHECK(run(box, game("", assets, "ctx.audio.play('v', { pan: 2 });")).find("audio pan must be in [-1, 1]") != std::string::npos);
  CHECK(run(box, game("", assets, "ctx.audio.stopMusic(1); check(ctx.audio.musicPlaying === null, 'stopped');")) == "");
  CHECK(run(box, game("", "sounds: { v: { file: 'a.ogg', bus: 'band' } }", "")).find("unknown bus \"band\"") != std::string::npos);
}

TEST_CASE("simulate seeds settings and saves in memory and reports music, locale, settings and saves") {
  Sandbox box;
  fs::copy_file(std::string(ALPHA_TEST_FIXTURES) + "/audio/tone.ogg", box.dir / "a.ogg", fs::copy_options::overwrite_existing);
  box.write("en.json", R"({ "hi": "Hello" })");
  box.write("pt.json", R"({ "hi": "Ola" })");
  const std::string g = box.write("game.js", game("", "music: { a: 'a.ogg' }, locales: { en: 'en.json', pt: 'pt.json' }",
      "ctx.audio.music('a'); ctx.save.set('runs', ctx.save.get('runs', 0) + 1);"
      "ctx.spawn({ tags: ['fx'], pos: [0, 0], emitter: { burst: 40, max: 40, lifetime: 5 } });"));
  Host host(box.save());
  const auto results = host.test(box.write("t.js",
      "import { simulate } from 'alpha/test'; import game from '" + fs::path(g).generic_string() + "';\n"
      "export function seeded() {\n"
      "  const r = simulate(game, { seconds: 0.1, settings: { locale: 'pt', reducedMotion: true }, saves: { main: { runs: 4 } } });\n"
      "  r.expect.music('a'); r.expect.particles().equals(10);\n"
      "  if (r.locale !== 'pt' || r.saves.main.runs !== 5 || !r.settings.reducedMotion) throw new Error(JSON.stringify(r));\n"
      "}\n"
      "export function plain() {\n"
      "  const r = simulate(game, { seconds: 0.1 });\n"
      "  r.expect.particles().equals(40);\n"
      "  if (r.locale !== 'en' || r.saves.main.runs !== 1 || r.settings.volumes.music !== 1) throw new Error(JSON.stringify(r));\n"
      "}\n"));
  REQUIRE(results.size() == 2);
  for (const auto& r : results) {
    const std::string what = r.name + ": " + r.error;
    CHECK_MESSAGE(r.passed(), what);
  }
  CHECK_FALSE(fs::exists(box.save_file()));
  CHECK_FALSE(fs::exists(box.dir / "settings.json"));
}
