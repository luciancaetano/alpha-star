#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <cmath>
#include <string>

#include "host_fixture.hpp"

using alpha::core::Region;
using alpha::core::Vec2;
using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

void bmp(const Sandbox& box, const std::string& name, int w, int h) {
  SDL_Surface* px = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
  SDL_SaveBMP(px, (box.dir / name).string().c_str());
  SDL_DestroySurface(px);
}

// A game with `assets` whose play scene runs `enter` and then `update` every step; size 100 x 100.
std::string game(const std::string& assets, const std::string& enter, const std::string& update = "") {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const check = (ok, what) => { if (!ok) throw new Error('failed: ' + what); };\n"
         "const play = scene('play', { enter(ctx) { " + enter + " }, update(ctx) { " + update + " } });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 1, actions: {} },\n"
         "  start: 'play', scenes: { play }, assets: " + assets + " });\n";
}

const std::string kSheets =
    "{ sounds: {}, sheets: { hero: { image: 'hero.bmp', frame: [8, 8], animations: {"
    " run: { frames: [0, 1, 2], fps: 10 }, jump: { frames: [3], loop: false } } } } }";

void steps(Host& host, int n) {
  for (int i = 0; i < n; ++i) host.step({}, kDt);
}

std::string boot_error(const Sandbox& box, const std::string& assets, const std::string& enter) {
  Host host(box.save());
  host.load(box.write("g.js", game(assets, enter)));
  return host.error();
}

}  // namespace

TEST_CASE("sheet sprites animate on the fixed step; scripts switch, restart and flip them") {
  Sandbox box;
  bmp(box, "hero.bmp", 32, 8);
  Host host(box.save());
  host.load(box.write("g.js", game(kSheets, R"(
    const e = ctx.spawn({ tags: ['hero'], pos: [50, 50], shape: { sheet: 'hero', animation: 'run' } });
    check(e.animation === 'run' && !e.flipX, 'starts running');
    e.flipX = true;
    check(e.flipX && !e.flipY, 'flip');
  )", "const e = ctx.find('hero'); if (ctx.time > 0.25 && e.animation === 'run') e.animation = 'jump';")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const auto src = [&] { return host.frame().shapes.at(0).shape.src; };
  CHECK(src() == Region{0, 0, 8, 8});
  CHECK(host.frame().shapes[0].shape.flip_x);
  CHECK(std::get<alpha::core::Rect>(host.frame().shapes[0].shape.geometry).size == Vec2{8, 8});
  steps(host, 7);
  CHECK(src() == Region{8, 0, 8, 8});
  steps(host, 12);
  CHECK(src() == Region{24, 0, 8, 8});  // jump
  CHECK(host.error() == "");
}

TEST_CASE("sheet mistakes name the sheet and list what exists") {
  Sandbox box;
  bmp(box, "hero.bmp", 32, 8);
  CHECK(boot_error(box, kSheets, "ctx.spawn({ tags: [], pos: [0, 0], shape: { sheet: 'villain' } });")
            .find("shape.sheet \"villain\" is not in assets.sheets; have: hero") != std::string::npos);
  CHECK(boot_error(box, kSheets, "ctx.spawn({ tags: [], pos: [0, 0], shape: { sheet: 'hero', animation: 'fly' } });")
            .find("have: jump run") != std::string::npos);
  CHECK(boot_error(box, kSheets, "ctx.spawn({ tags: [], pos: [0, 0], shape: { sheet: 'hero' } }).animation = 'fly';")
            .find("animations: jump, run") != std::string::npos);
  CHECK(boot_error(box, "{ sheets: { s: { image: 'hero.bmp', frame: [8, 8], animations: { a: { frames: [4] } } } } }", "")
            .find("frame 4 is outside the sheet (4 frames, 0..3)") != std::string::npos);
  CHECK(boot_error(box, "{ sheets: { s: { image: 'hero.bmp', frame: [16, 16] } } }", "")
            .find("does not fit the image (32x8)") != std::string::npos);
  CHECK(boot_error(box, "{ sheets: { s: { frame: [8, 8] } } }", "")
            .find("assets.sheets.s.image must be an image file path") != std::string::npos);
}

TEST_CASE("an exported pack's alpha-atlas.json is transparent: images and sheets become regions of a page") {
  Sandbox box;
  bmp(box, "atlas-0.bmp", 64, 64);
  box.write("alpha-atlas.json", R"({ "filters": { "nearest": { "img/coin.png": ["atlas-0.bmp", [32, 0, 4, 4]],
                                            "img/hero.png": ["atlas-0.bmp", [0, 16, 32, 8]] } } })");
  Host host(box.save());
  host.load(box.write("g.js", game(
      "{ images: { coin: 'img/coin.png' }, sheets: { hero: { image: 'img/hero.png', frame: [8, 8] } } }",
      "ctx.spawn({ tags: [], pos: [10, 10], shape: { image: 'coin' } });"
      "ctx.spawn({ tags: [], pos: [10, 10], z: 1, shape: { sheet: 'hero' } });")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const auto shapes = host.frame().shapes;
  CHECK(shapes[0].shape.src == Region{32, 0, 4, 4});
  CHECK(shapes[1].shape.src == Region{0, 16, 8, 8});
  CHECK(shapes[0].shape.image == shapes[1].shape.image);  // one texture
}

TEST_CASE("ctx.camera follows, zooms, maps screen to world and reaches the renderer's view") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game("{}", R"(
    const hero = ctx.spawn({ tags: ['hero'], pos: [500, 50] });
    check(ctx.camera.pos.x === 50 && ctx.camera.zoom === 1, 'identity camera');
    ctx.camera.follow(hero, { lerp: 1 });
  )", R"(
    if (ctx.time > 0.01) {
      const w = ctx.camera.toWorld([0, 0]);
      check(w[0] === 450 && w[1] === 0, 'toWorld ' + w);
      ctx.camera.zoom = 2;
    }
  )")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  steps(host, 1);
  CHECK(host.frame().view->center == Vec2{500, 50});
  steps(host, 1);
  CHECK(host.frame().view->zoom == 2.0f);
  CHECK(host.error() == "");
}

TEST_CASE("camera mistakes are errors for the agent") {
  Sandbox box;
  CHECK(boot_error(box, "{}", "ctx.camera.zoom = 0;").find("camera.zoom must be in") != std::string::npos);
  CHECK(boot_error(box, "{}", "ctx.camera.bounds([0, 0]);").find("[x, y, width, height] or null") != std::string::npos);
  CHECK(boot_error(box, "{}", "ctx.camera.follow({}, {});").find("entity must be an entity") != std::string::npos);
  CHECK(boot_error(box, "{}", "ctx.camera.follow(ctx.spawn({ tags: [], pos: [0, 0] }), { lerp: 2 });")
            .find("lerp must be in") != std::string::npos);
  CHECK(boot_error(box, "{}", "ctx.camera.shake(-1, 1);").find("intensity must be in") != std::string::npos);
}

TEST_CASE("camera shake is deterministic across runs and bounds clamp the view") {
  Sandbox box;
  const std::string src = game("{}", "ctx.camera.bounds([0, 0, 400, 100]); ctx.camera.pos.x = 1000; ctx.camera.shake(6, 1);");
  Host a(box.save());
  Host b(box.save());
  a.load(box.write("a.js", src));
  b.load(box.write("b.js", src));
  steps(a, 10);
  steps(b, 10);
  CHECK(a.frame().view->center == b.frame().view->center);
  CHECK(a.frame().view->center.x != 350.0f);  // shaking
  CHECK(std::abs(a.frame().view->center.x - 350.0f) <= 6.0f);
}

TEST_CASE("ctx.input.mouse reports the pointer in game pixels") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game("{}", "", R"(
    const [x, y] = ctx.input.mouse();
    if (ctx.time > 0) check(x === 12 && y === 34, 'mouse ' + x + ',' + y);
  )")));
  alpha::core::InputFrame frame;
  frame.mouse = {12, 34};
  host.step(frame, kDt);
  host.step(frame, kDt);
  CHECK(host.error() == "");
}
