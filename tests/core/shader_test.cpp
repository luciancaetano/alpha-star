#include <doctest/doctest.h>

#include <functional>
#include <limits>
#include <memory>
#include <string>

#include "core/error.hpp"
#include "core/postfx.hpp"
#include "core/shader.hpp"

using namespace alpha::core;

namespace {

std::string error_of(const std::function<void()>& fn) {
  try {
    fn();
  } catch (const Error& e) {
    return e.what();
  }
  return "";
}

ShaderPtr wave() {
  return std::make_shared<const Shader>(make_shader("wave", "fx/wave.hlsl", {{"amount", 0.5f}, {"time", 0.0f}}));
}

}  // namespace

TEST_CASE("make_shader keeps uniforms in order and validates the entry") {
  const Shader s = make_shader("wave", "fx/wave.hlsl", {{"amount", 0.5f}, {"time", 2.0f}});
  CHECK(s.uniforms[1].first == "time");
  CHECK(error_of([] { make_shader("crt", "a.hlsl", {}); }) ==
        "assets.shaders.crt: \"crt\" is a built-in postfx effect; pick another name");
  CHECK(error_of([] { make_shader("w", "a.glsl", {}); }) ==
        "assets.shaders.w.file must be an HLSL fragment shader (.hlsl), got \"a.glsl\"");
  std::vector<std::pair<std::string, float>> many(kMaxShaderUniforms + 1, {"u", 1.0f});
  CHECK(error_of([&] { make_shader("w", "a.hlsl", many); }) == "assets.shaders.w.uniforms: 17 uniforms; at most 16 floats");
  const float inf = std::numeric_limits<float>::infinity();
  CHECK(error_of([&] { make_shader("w", "a.hlsl", {{"x", inf}}); }).find("assets.shaders.w.uniforms.x must be a finite") == 0);
}

TEST_CASE("blob files sit next to the source, one per backend format") {
  CHECK(blob_file("fx/wave.hlsl", ShaderFormat::Spirv) == "fx/wave.spv");
  CHECK(blob_file("fx/wave.hlsl", ShaderFormat::Dxil) == "fx/wave.dxil");
  CHECK(blob_file("fx/wave.hlsl", ShaderFormat::Msl) == "fx/wave.msl");
  CHECK(format_name(ShaderFormat::Msl) == "msl");
  CHECK(format_name(ShaderFormat::Spirv) == "spirv");
}

TEST_CASE("pick_format takes the first format both the device and the shader have") {
  const ShaderBlobs all{{ShaderFormat::Spirv, {1}}, {ShaderFormat::Dxil, {2}}, {ShaderFormat::Msl, {3}}};
  CHECK(pick_format(format_bit(ShaderFormat::Dxil), all) == ShaderFormat::Dxil);
  CHECK(pick_format(format_bit(ShaderFormat::Msl), all) == ShaderFormat::Msl);
  const std::uint32_t both = format_bit(ShaderFormat::Spirv) | format_bit(ShaderFormat::Dxil);
  CHECK(pick_format(both, all) == ShaderFormat::Spirv);
  CHECK(pick_format(both, {{ShaderFormat::Dxil, {2}}}) == ShaderFormat::Dxil);
  CHECK_FALSE(pick_format(format_bit(ShaderFormat::Msl), {{ShaderFormat::Dxil, {2}}}).has_value());
  CHECK_FALSE(pick_format(0, all).has_value());
}

TEST_CASE("shader effects: defaults, params by name, agent-facing errors, chain by name") {
  const ShaderPtr w = wave();
  const Effect e = make_shader_effect(w, {{"time", 3.0f}});
  CHECK(e.kind == EffectKind::Shader);
  CHECK(e.uniforms[0] == 0.5f);
  CHECK(e.uniforms[1] == 3.0f);
  CHECK(effect_name(e) == "wave");
  CHECK(error_of([&] { make_shader_effect(w, {{"speed", 1.0f}}); }) ==
        "postfx \"wave\": unknown uniform \"speed\"; have: amount time");
  CHECK(needs_scene({e}));
  PostFx fx;
  fx.put(make_effect("crt", {}));
  fx.put(e);
  fx.put(make_shader_effect(w, {{"amount", 1.0f}}));
  REQUIRE(fx.chain().size() == 2);
  CHECK(fx.chain()[1].uniforms[0] == 1.0f);
  CHECK(fx.remove("wave"));
  CHECK(fx.chain().size() == 1);
}
