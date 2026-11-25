#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>

#include "host_fixture.hpp"

using alpha::core::Blend;
using alpha::core::DrawItem;
using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

// A 100 x 100 game whose play scene runs `enter` once and `update` every step.
std::string game(const std::string& config, const std::string& enter, const std::string& update = "",
                 const std::string& assets = "{}") {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const check = (ok, what) => { if (!ok) throw new Error('failed: ' + what); };\n"
         "const play = scene('play', { enter(ctx) { " + enter + " }, update(ctx) { " + update + " } });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 3, actions: {} " + config + " },\n"
         "  start: 'play', scenes: { play }, assets: " + assets + " });\n";
}

void steps(Host& host, int n) {
  for (int i = 0; i < n; ++i) host.step({}, kDt);
}

std::string error_of(const Sandbox& box, const std::string& config, const std::string& enter,
                     const std::string& assets = "{}") {
  Host host(box.save());
  host.load(box.write("g.js", game(config, enter, "", assets)));
  return host.error();
}

std::string font_assets(const Sandbox& box) {
  std::filesystem::copy_file(ALPHA_TEST_FONT, box.dir / "font.ttf", std::filesystem::copy_options::overwrite_existing);
  return "{ fonts: { pixel: 'font.ttf' } }";
}

}  // namespace

TEST_CASE("emitters burst on the first step, fly, die, and one-shots destroy their entity") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game("", R"(
    ctx.spawn({ tags: ['dust'], pos: [50, 50],
      emitter: { burst: 12, lifetime: 0.25, speed: [20, 40], angle: [180, 360], size: [3, 0], color: ['#ffffffff', '#ffffff00'], oneShot: true } });
  )")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  steps(host, 1);
  CHECK(host.world()->particle_count() == 12);
  const auto items = host.world()->draw_items();
  REQUIRE(items.size() == 1);
  CHECK(items[0].particles.size() == 12);
  steps(host, 16);
  CHECK(host.world()->particle_count() == 0);
  CHECK(host.world()->entities().empty());  // the one-shot is gone
}

TEST_CASE("entity.emit bursts on demand; emitting switches the rate; counts are visible") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game("", R"(
    const bird = ctx.spawn({ tags: ['bird'], pos: [50, 50], shape: { circle: 4, color: '#fff' }, emitter: { rate: 0, lifetime: 1 } });
    bird.emit(8);
  )", R"(
    const bird = ctx.find('bird');
    if (ctx.time > 0 && ctx.time < 0.02) check(bird.particles === 8, 'burst of 8, got ' + bird.particles);
  )")));
  steps(host, 2);
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(host.world()->particle_count() == 8);
}

TEST_CASE("particles use the game's seeded random: same seed, same picture") {
  Sandbox box;
  const std::string src = game("", "ctx.spawn({ tags: ['p'], pos: [50, 50], emitter: { rate: 120, speed: [5, 50] } });");
  Host a(box.save());
  a.load(box.write("a.js", src));
  Host b(box.save());
  b.load(box.write("b.js", src));
  steps(a, 30);
  steps(b, 30);
  const auto ia = a.world()->draw_items();
  const auto ib = b.world()->draw_items();
  REQUIRE(ia.size() == 1);
  REQUIRE(ia[0].particles.size() == ib[0].particles.size());
  for (std::size_t i = 0; i < ia[0].particles.size(); ++i) CHECK(ia[0].particles[i].pos == ib[0].particles[i].pos);
}

TEST_CASE("emitter errors name the field and what is allowed") {
  Sandbox box;
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], emitter: { max: 50000 } });").find("emitter.max must be 1..20000") != std::string::npos);
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], emitter: { shape: 'star' } });").find("\"circle\", \"rect\"") != std::string::npos);
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], emitter: { oneShot: true } });").find("oneShot needs a burst") != std::string::npos);
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], emitter: { color: 'red' } });").find("emitter.color must be a color") != std::string::npos);
  CHECK(error_of(box, "", "ctx.find; ctx.spawn({ tags: ['a'], pos: [0, 0] }).emit(3);").find("has no emitter") != std::string::npos);
}

TEST_CASE("particles can be sheet frames or images; a bad frame lists the range") {
  Sandbox box;
  SDL_Surface* px = SDL_CreateSurface(32, 8, SDL_PIXELFORMAT_RGBA32);
  SDL_SaveBMP(px, (box.dir / "s.bmp").string().c_str());
  SDL_DestroySurface(px);
  const std::string assets = "{ images: { dot: 's.bmp' }, sheets: { fx: { image: 's.bmp', frame: [8, 8] } } }";
  Host host(box.save());
  host.load(box.write("g.js", game("", R"(
    ctx.spawn({ tags: ['a'], pos: [50, 50], emitter: { burst: 2, shape: { sheet: 'fx', frame: 3 } } });
    ctx.spawn({ tags: ['b'], pos: [50, 50], emitter: { burst: 2, shape: { image: 'dot' } } });
  )", "", assets)));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  steps(host, 1);
  const auto items = host.world()->draw_items();
  REQUIRE(items.size() == 2);
  CHECK(items[0].shape.src == alpha::core::Region{24, 0, 8, 8});
  CHECK(items[1].shape.image);
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], emitter: { shape: { sheet: 'fx', frame: 4 } } });", assets)
            .find("emitter.shape.frame must be 0..3") != std::string::npos);
}

TEST_CASE("tint, alpha and blend: spawned with the shape, settable at runtime") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game("", R"(
    const e = ctx.spawn({ tags: ['e'], pos: [50, 50], shape: { rect: [10, 10], color: '#ffffff', tint: '#ff0000', alpha: 0.5, blend: 'add' } });
    check(e.tint === '#ff0000ff' && e.alpha === 0.5 && e.blend === 'add', 'spawned look ' + e.tint + ' ' + e.alpha + ' ' + e.blend);
    e.tint = '#00ff0080';
    e.alpha = 0.25;
    e.blend = 'multiply';
  )")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const DrawItem item = host.world()->draw_items().at(0);
  CHECK(item.shape.tint == alpha::core::Color{0, 255, 0, 128});
  CHECK(item.shape.alpha == 0.25f);
  CHECK(item.shape.blend == Blend::Multiply);
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], shape: { circle: 1, color: '#fff', blend: 'screen' } });").find("\"alpha\", \"add\" or \"multiply\"") != std::string::npos);
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], shape: { circle: 1, color: '#fff' } }).alpha = 2;").find("alpha must be 0..1") != std::string::npos);
}

TEST_CASE("layers order drawing before entity z and carry their parallax") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game(", layers: { bg: { z: -10, parallax: 0.25 }, fx: { z: 5 } }", R"(
    ctx.spawn({ tags: ['front'], pos: [1, 1], z: 100, shape: { circle: 1, color: '#fff' } });
    ctx.spawn({ tags: ['sky'], pos: [2, 2], z: 100, layer: 'bg', shape: { circle: 1, color: '#fff' } });
    const fx = ctx.spawn({ tags: ['fx'], pos: [3, 3], z: -100, layer: 'fx', shape: { circle: 1, color: '#fff' } });
    check(fx.layer === 'fx', 'layer name');
  )")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const auto items = host.world()->draw_items();
  REQUIRE(items.size() == 3);
  CHECK(items[0].pos.x == 2.0f);
  CHECK(items[0].parallax == 0.25f);
  CHECK(items[1].pos.x == 1.0f);
  CHECK(items[2].pos.x == 3.0f);
  CHECK(error_of(box, ", layers: { bg: { z: 0 } }", "ctx.spawn({ tags: [], pos: [0, 0], layer: 'sky' });").find("\"sky\" is not in config.layers; have: bg") != std::string::npos);
  CHECK(error_of(box, ", layers: { bg: { parallax: 1 } }", "").find("config.layers.bg must be { z: number") != std::string::npos);
}

TEST_CASE("world text: a font draws glyph quads; text is settable; no font uses the debug font") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game("", R"(
    const t = ctx.spawn({ tags: ['t'], pos: [50, 50], shape: { text: '+1', font: 'pixel', size: 8, color: '#ffd700' } });
    check(t.text === '+1', 'text');
    ctx.spawn({ tags: ['d'], pos: [50, 80], shape: { text: 'debug', size: 16, align: 'left' } });
  )", "ctx.find('t').text = 'score 10';", font_assets(box))));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  steps(host, 1);
  const auto items = host.world()->draw_items();
  REQUIRE(items.size() == 2);
  REQUIRE(items[0].shape.image);
  REQUIRE(items[0].shape.tiles);
  CHECK(items[0].shape.tiles->tiles.size() == 7);  // "score 10" without the space
  CHECK(items[0].shape.text->text == "score 10");
  CHECK_FALSE(items[1].shape.image);
  CHECK(std::get<alpha::core::Rect>(items[1].shape.geometry).size.x == 160.0f);  // 5 x 16, doubled for left align
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], shape: { text: 'x', font: 'nope' } });", font_assets(box))
            .find("\"nope\" is not in assets.fonts; have: pixel") != std::string::npos);
}

TEST_CASE("ctx.window: fullscreen requests and size; config.window is parsed and checked") {
  Sandbox box;
  Host host(box.save());
  const auto info = host.load(box.write("g.js", game(", window: { scaling: 'integer', fullscreen: true, altEnter: false }", R"(
    check(ctx.window.size[0] === 100 && ctx.window.size[1] === 100, 'size ' + ctx.window.size);
    check(!ctx.window.fullscreen, 'starts windowed');
    ctx.window.fullscreen = true;
    check(ctx.window.fullscreen, 'requested');
  )")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(info.window.scaling == alpha::core::Scaling::Integer);
  CHECK(info.window.fullscreen);
  CHECK_FALSE(info.window.alt_enter);
  CHECK(host.window().request == true);
  CHECK(error_of(box, ", window: { scaling: 'fit' }", "").find("config.window.scaling must be \"letterbox\"") != std::string::npos);
}

TEST_CASE("expand: the scene camera follows the logical size") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game("", "")));
  host.resize_view({160, 100});
  CHECK(host.world()->view().size == alpha::core::Vec2{160, 100});
}
