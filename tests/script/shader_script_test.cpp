#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>

#include "host_fixture.hpp"

using alpha::core::EffectKind;
using alpha::core::ShaderFormat;
using alpha::script::Host;
using alpha::test::Sandbox;

// assets.shaders + ctx.postfx with a custom shader, as a script sees them.
namespace {

const std::filesystem::path kFixtures = std::filesystem::path(ALPHA_TEST_FIXTURES) / "shaders";

std::string game(const std::string& shaders, const std::string& enter) {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { enter(ctx) { " + enter + " } });\n"
         "export default game({ config: { title: 't', size: [64, 32], actions: {} },\n"
         "  assets: { shaders: " + shaders + " }, start: 'play', scenes: { play } });\n";
}

// The fixture's compiled blobs (no .hlsl: a pack as alpha export writes it).
void pack_blobs(const Sandbox& box, const std::string& exts) {
  std::filesystem::create_directories(box.dir / "fx");
  for (const std::string ext : {".spv", ".dxil", ".msl"}) {
    if (exts.find(ext) == std::string::npos) continue;
    std::filesystem::copy_file(kFixtures / ("invert" + ext), box.dir / "fx" / ("invert" + ext),
                               std::filesystem::copy_options::overwrite_existing);
  }
}

const std::string kInvert = "{ invert: { file: 'fx/invert.hlsl', uniforms: { amount: 1 } } }";

std::string error_of(const Sandbox& box, const std::string& shaders, const std::string& enter) {
  Host host(box.save());
  host.load(box.write("g.js", game(shaders, enter)));
  return host.error();
}

}  // namespace

TEST_CASE("assets.shaders loads packed blobs; postfx.add runs it with its uniforms") {
  Sandbox box;
  pack_blobs(box, ".spv.dxil.msl");
  Host host(box.save());
  const auto info = host.load(box.write("g.js", game(kInvert, "ctx.postfx.add('crt'); ctx.postfx.add('invert', { amount: 0.25 });")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(info.shaders);
  const auto frame = host.frame();
  REQUIRE(frame.effects.size() == 2);
  CHECK(frame.effects[1].kind == EffectKind::Shader);
  CHECK(frame.effects[1].uniforms[0] == doctest::Approx(0.25));
  CHECK(frame.effects[1].shader->blobs.size() == 3);
}

TEST_CASE("only the blobs a pack has are loaded") {
  Sandbox box;
  pack_blobs(box, ".dxil");
  Host host(box.save());
  host.load(box.write("g.js", game(kInvert, "ctx.postfx.add('invert');")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const auto& blobs = host.frame().effects.at(0).shader->blobs;
  CHECK(blobs.size() == 1);
  CHECK(blobs.contains(ShaderFormat::Dxil));
}

TEST_CASE("shader errors are written for the agent") {
  Sandbox box;
  pack_blobs(box, ".spv");
  CHECK(error_of(box, "{ gone: { file: 'fx/gone.hlsl' } }", "")
            .find("assets.shaders.gone: no compiled shader for \"fx/gone.hlsl\" (looked for fx/gone.spv, .dxil, .msl") !=
        std::string::npos);
  CHECK(error_of(box, kInvert, "ctx.postfx.add('invert', { speed: 1 });")
            .find("postfx \"invert\": unknown uniform \"speed\"; have: amount") != std::string::npos);
  CHECK(error_of(box, kInvert, "ctx.postfx.add('blur');").find("have: bloom crt pixelate tint vignette invert") !=
        std::string::npos);
  CHECK(error_of(box, "{ bad: 'fx/invert.hlsl' }", "").find("assets.shaders.bad must be { file") != std::string::npos);
  CHECK(error_of(box, "{ vignette: { file: 'fx/invert.hlsl' } }", "").find("is a built-in postfx effect") !=
        std::string::npos);
  CHECK(error_of(box, kInvert, "ctx.postfx.add('invert'); if (ctx.postfx.remove('invert') !== true) throw new Error('rm');")
            .empty());
}

#ifdef ALPHA_TEST_SHADERCROSS
TEST_CASE("debug engines compile the .hlsl with shadercross; compile errors reach the agent") {
  Sandbox box;
  std::filesystem::create_directories(box.dir / "fx");
  std::filesystem::copy_file(kFixtures / "invert.hlsl", box.dir / "fx" / "invert.hlsl");
  box.write("fx/broken.hlsl", "float4 main() : SV_Target { return nope; }\n");
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_SHADERCROSS", ALPHA_TEST_SHADERCROSS, true);
  {
    Host host(box.save());
    host.load(box.write("g.js", game(kInvert, "ctx.postfx.add('invert');")));
    REQUIRE_MESSAGE(!host.paused(), host.error());
    CHECK(host.frame().effects.at(0).shader->blobs.size() == 3);
    const std::string bad = error_of(box, "{ broken: { file: 'fx/broken.hlsl' } }", "");
    CHECK(bad.find("assets.shaders.broken: broken.hlsl did not compile") != std::string::npos);
    CHECK(bad.find("nope") != std::string::npos);
  }
  SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_SHADERCROSS");
}
#endif
