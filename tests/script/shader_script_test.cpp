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

// The fixture's compiled SPIR-V and MSL (no .frag: a pack as alpha export writes it).
void pack_blobs(const Sandbox& box) {
  std::filesystem::create_directories(box.dir / "fx");
  for (const char* blob : {"invert.spv", "invert.msl"}) {
    std::filesystem::copy_file(kFixtures / blob, box.dir / "fx" / blob, std::filesystem::copy_options::overwrite_existing);
  }
}

const std::string kInvert = "{ invert: { file: 'fx/invert.frag', uniforms: { amount: 1 } } }";

std::string error_of(const Sandbox& box, const std::string& shaders, const std::string& enter) {
  Host host(box.save());
  host.load(box.write("g.js", game(shaders, enter)));
  return host.error();
}

}  // namespace

TEST_CASE("assets.shaders loads packed blobs; postfx.set runs it with its uniforms") {
  Sandbox box;
  pack_blobs(box);
  Host host(box.save());
  const auto info = host.load(box.write("g.js", game(kInvert, "ctx.postfx.set('crt'); ctx.postfx.set('invert', { amount: 0.25 });")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(info.shaders);
  const auto frame = host.frame();
  REQUIRE(frame.effects.size() == 2);
  CHECK(frame.effects[1].kind == EffectKind::Shader);
  CHECK(frame.effects[1].uniforms[0] == doctest::Approx(0.25));
  CHECK(frame.effects[1].shader->blobs.contains(ShaderFormat::Spirv));
  CHECK(frame.effects[1].shader->blobs.contains(ShaderFormat::Msl));  // Metal devices pick this one
}

TEST_CASE("shader errors are written for the agent") {
  Sandbox box;
  pack_blobs(box);
  CHECK(error_of(box, "{ gone: { file: 'fx/gone.frag' } }", "")
            .find("assets.shaders.gone: no compiled shader for \"fx/gone.frag\" (looked for fx/gone.spv") !=
        std::string::npos);
  CHECK(error_of(box, kInvert, "ctx.postfx.set('invert', { speed: 1 });")
            .find("postfx \"invert\": unknown uniform \"speed\"; have: amount") != std::string::npos);
  CHECK(error_of(box, kInvert, "ctx.postfx.set('blur');").find("have: bloom crt pixelate tint vignette invert") !=
        std::string::npos);
  CHECK(error_of(box, "{ bad: 'fx/invert.frag' }", "").find("assets.shaders.bad must be { file") != std::string::npos);
  CHECK(error_of(box, "{ vignette: { file: 'fx/invert.frag' } }", "").find("is a built-in postfx effect") !=
        std::string::npos);
  CHECK(error_of(box, kInvert, "ctx.postfx.set('invert'); if (ctx.postfx.remove('invert') !== true) throw new Error('rm');")
            .empty());
}

#ifdef ALPHA_TEST_GLSLANG
TEST_CASE("debug engines compile the .frag with glslang; compile errors name file and line") {
  Sandbox box;
  std::filesystem::create_directories(box.dir / "fx");
  std::filesystem::copy_file(kFixtures / "invert.frag", box.dir / "fx" / "invert.frag");
  box.write("fx/broken.frag", "#version 450\nlayout(location = 0) out vec4 o_color;\nvoid main() { o_color = nope; }\n");
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_GLSLANG", ALPHA_TEST_GLSLANG, true);
  {
    Host host(box.save());
    host.load(box.write("g.js", game(kInvert, "ctx.postfx.set('invert');")));
    REQUIRE_MESSAGE(!host.paused(), host.error());
    const auto& blobs = host.frame().effects.at(0).shader->blobs;
    REQUIRE(blobs.contains(ShaderFormat::Spirv));
    CHECK(blobs.at(ShaderFormat::Spirv).at(0) == 0x03);  // SPIR-V magic 0x07230203, little-endian
    const std::string bad = error_of(box, "{ broken: { file: 'fx/broken.frag' } }", "");
    CAPTURE(bad);
    CHECK(bad.find("assets.shaders.broken: fx/broken.frag did not compile") != std::string::npos);
    CHECK(bad.find("fx/broken.frag:3: 'nope' : undeclared identifier") != std::string::npos);
  }
  SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_GLSLANG");
}
#endif

#if defined(ALPHA_TEST_GLSLANG) && defined(ALPHA_TEST_SHADERCROSS)
TEST_CASE("debug engines also turn the SPIR-V into MSL with shadercross (Metal entry point main0)") {
  Sandbox box;
  std::filesystem::create_directories(box.dir / "fx");
  std::filesystem::copy_file(kFixtures / "invert.frag", box.dir / "fx" / "invert.frag");
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_GLSLANG", ALPHA_TEST_GLSLANG, true);
  SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_SHADERCROSS", ALPHA_TEST_SHADERCROSS, true);
  {
    Host host(box.save());
    host.load(box.write("g.js", game(kInvert, "ctx.postfx.set('invert');")));
    REQUIRE_MESSAGE(!host.paused(), host.error());
    const auto& blobs = host.frame().effects.at(0).shader->blobs;
    REQUIRE(blobs.contains(ShaderFormat::Msl));
    const std::string msl(blobs.at(ShaderFormat::Msl).begin(), blobs.at(ShaderFormat::Msl).end());
    CHECK(msl.find("#include <metal_stdlib>") != std::string::npos);
    CHECK(msl.find("fragment main0_out main0(") != std::string::npos);
    CHECK(msl.find("float amount;") != std::string::npos);  // the uniform block survives
  }
  SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_SHADERCROSS");
  SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), "ALPHA_GLSLANG");
}
#endif
