#include <doctest/doctest.h>

#include <string>

#include "host_fixture.hpp"

using alpha::core::EffectKind;
using alpha::script::Host;
using alpha::test::Sandbox;

// ctx.postfx, config.lighting and spawn `light` as a script sees them.
namespace {

constexpr float kDt = 1.0f / 60.0f;

std::string game(const std::string& config, const std::string& enter, const std::string& update = "") {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const check = (ok, what) => { if (!ok) throw new Error('failed: ' + what); };\n"
         "const other = scene('other', {});\n"
         "const play = scene('play', { enter(ctx) { " + enter + " }, update(ctx) { " + update + " } });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 3, actions: {} " + config + " },\n"
         "  start: 'play', scenes: { play, other } });\n";
}

std::string error_of(const Sandbox& box, const std::string& config, const std::string& enter) {
  Host host(box.save());
  host.load(box.write("g.js", game(config, enter)));
  return host.error();
}

}  // namespace

TEST_CASE("postfx.add builds the chain in order; set/remove/list; frames carry it") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game("", R"(
    ctx.postfx.add('bloom', { strength: 1.2 });
    ctx.postfx.add('crt');
    ctx.postfx.set('bloom', { strength: 0.3 });
    check(ctx.postfx.list().join() === 'bloom,crt', 'order');
    check(ctx.postfx.remove('crt') === true && ctx.postfx.remove('crt') === false, 'remove');
    ctx.postfx.add('tint', { color: '#ff0000', strength: 0.5 });
  )")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const auto frame = host.frame();
  REQUIRE(frame.effects.size() == 2);
  CHECK(frame.effects[0].kind == EffectKind::Bloom);
  CHECK(frame.effects[0].strength == doctest::Approx(0.3));
  CHECK(frame.effects[1].color == alpha::core::Color{255, 0, 0, 255});
  CHECK_FALSE(frame.lighting.has_value());
}

TEST_CASE("postfx is scene-scoped: a new scene starts without effects") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game("", "ctx.postfx.add('vignette');", "ctx.go('other');")));
  REQUIRE(host.frame().effects.size() == 1);
  host.step({}, kDt);
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(host.frame().effects.empty());
}

TEST_CASE("postfx errors are written for the agent") {
  Sandbox box;
  CHECK(error_of(box, "", "ctx.postfx.add('blur');").find("postfx: unknown effect \"blur\"; have: bloom crt pixelate tint vignette") !=
        std::string::npos);
  CHECK(error_of(box, "", "ctx.postfx.add('crt', { amount: 1 });").find("unknown param \"amount\"; have: strength") !=
        std::string::npos);
  CHECK(error_of(box, "", "ctx.postfx.add('vignette', { strength: 5 });").find("postfx \"vignette\".strength must be 0") !=
        std::string::npos);
  CHECK(error_of(box, "", "ctx.postfx.set('crt', {});").find("not added in this scene") != std::string::npos);
}

TEST_CASE("lights ride on entities; config.lighting sets the ambient") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game(", lighting: { ambient: '#102030' }", R"(
    const torch = ctx.spawn({ tags: [], pos: [10, 20], light: { radius: 40, color: '#ffcc88', intensity: 2 } });
    ctx.spawn({ tags: [], pos: [0, 0], shape: { circle: 3, color: '#fff' } });
    torch.pos.x = 30;
  )")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  host.step({}, kDt);
  const auto frame = host.frame();
  REQUIRE(frame.lighting.has_value());
  CHECK(frame.lighting->ambient == alpha::core::Color{0x10, 0x20, 0x30, 255});
  REQUIRE(frame.lights.size() == 1);
  CHECK(frame.lights[0].pos.x == doctest::Approx(30));
  CHECK(frame.lights[0].light.radius == doctest::Approx(40));
  CHECK(frame.lights[0].light.intensity == doctest::Approx(2));
}

TEST_CASE("light and lighting errors name the field") {
  Sandbox box;
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], light: {} });").find("spawn: light.radius is required") != std::string::npos);
  CHECK(error_of(box, "", "ctx.spawn({ tags: [], pos: [0, 0], light: { radius: -1 } });").find("spawn: light.radius must be 1..4096") !=
        std::string::npos);
  CHECK(error_of(box, ", lighting: { ambient: 3 }", "").find("config.lighting must be { ambient") != std::string::npos);
  CHECK(error_of(box, ", lighting: { ambient: 'dark' }", "").find("config.lighting.ambient must be a color") != std::string::npos);
}
