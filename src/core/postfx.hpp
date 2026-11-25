#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/color.hpp"

// Built-in full-screen post effects (ctx.postfx): data and validation only;
// render::Compositor draws them. Render-only: never read by simulate().
namespace alpha::core {

enum class EffectKind { Bloom, Crt, Pixelate, Tint, Vignette };

// One effect with every parameter resolved (defaults filled in).
// bloom: strength 0..2, radius 1..16 game px (blur); crt: strength 0..1
// (scanlines + vignette); pixelate: size 1..64 game px per block;
// tint: color multiplied in by strength 0..1; vignette: strength 0..1,
// radius 0..1 (clear part of the screen, from the center).
struct Effect {
  EffectKind kind = EffectKind::Vignette;
  float strength = 0.5f;
  float radius = 0.5f;
  float size = 4.0f;
  Color color{255, 255, 255, 255};
  bool operator==(const Effect&) const = default;
};

using EffectNumbers = std::map<std::string, float, std::less<>>;

// The effect `name` with `numbers` (and `color`, tint only) over its
// defaults. Throws Error naming the effect, the bad parameter and what is
// allowed (unknown effect, unknown parameter, value out of range).
Effect make_effect(std::string_view name, const EffectNumbers& numbers, std::optional<Color> color = {});
std::string_view effect_name(EffectKind kind);
std::optional<EffectKind> effect_kind(std::string_view name);
// "bloom crt pixelate tint vignette": for error messages.
std::string effect_names();
// True when the chain needs the frame as a texture (bloom, pixelate).
bool needs_scene(const std::vector<Effect>& chain);

// A scene's effect chain, drawn in order. Adding a kind already in the chain
// replaces its parameters in place (the order stays the order of first add).
class PostFx {
 public:
  void put(const Effect& effect);
  bool remove(EffectKind kind);
  void clear() { chain_.clear(); }
  const std::vector<Effect>& chain() const { return chain_; }

 private:
  std::vector<Effect> chain_;
};

}  // namespace alpha::core
