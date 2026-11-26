#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/color.hpp"
#include "core/shader.hpp"

// Built-in full-screen post effects (ctx.postfx): data and validation only;
// render::Compositor draws them. Render-only: never read by simulate().
namespace alpha::core {

enum class EffectKind { Bloom, Crt, Pixelate, Tint, Vignette, Shader };

// One effect with every parameter resolved (defaults filled in).
// bloom: strength 0..2, radius 1..16 game px (blur); crt: strength 0..1
// (scanlines + vignette); pixelate: size 1..64 game px per block;
// tint: color multiplied in by strength 0..1; vignette: strength 0..1,
// radius 0..1 (clear part of the screen, from the center).
// shader: a custom assets.shaders entry run over the frame with `uniforms`.
struct Effect {
  EffectKind kind = EffectKind::Vignette;
  float strength = 0.5f;
  float radius = 0.5f;
  float size = 4.0f;
  Color color{255, 255, 255, 255};
  ShaderPtr shader{};
  ShaderUniforms uniforms{};
  bool operator==(const Effect&) const = default;
};

using EffectNumbers = std::map<std::string, float, std::less<>>;

// The effect `name` with `numbers` (and `color`, tint only) over its
// defaults. Throws Error naming the effect, the bad parameter and what is
// allowed (unknown effect, unknown parameter, value out of range).
Effect make_effect(std::string_view name, const EffectNumbers& numbers, std::optional<Color> color = {});
// A custom shader effect: the shader's uniform defaults, then `numbers`.
Effect make_shader_effect(const ShaderPtr& shader, const EffectNumbers& numbers);
std::string_view effect_name(EffectKind kind);
// The name scripts use: the built-in's, or the shader's assets.shaders name.
std::string_view effect_name(const Effect& effect);
std::optional<EffectKind> effect_kind(std::string_view name);
// "bloom crt pixelate tint vignette": for error messages.
std::string effect_names();
// True when the chain needs the frame as a texture (bloom, pixelate, shaders).
bool needs_scene(const std::vector<Effect>& chain);

// A scene's effect chain, drawn in order. Adding an effect already in the chain
// (same name) replaces its parameters in place (the order stays the order of first add).
class PostFx {
 public:
  void put(const Effect& effect);
  bool remove(std::string_view name);
  void clear() { chain_.clear(); }
  const std::vector<Effect>& chain() const { return chain_; }

 private:
  std::vector<Effect> chain_;
};

}  // namespace alpha::core
