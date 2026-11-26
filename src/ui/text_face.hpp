#pragma once

#include <RmlUi/Core/CallbackTexture.h>
#include <RmlUi/Core/FontMetrics.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/StyleTypes.h>
#include <RmlUi/Core/Types.h>

#include <deque>

#include "core/glyph_layout.hpp"

namespace Rml {
class RenderManager;
}

namespace alpha::ui {

// Game UI text in one font (any font source: TrueType with its fallbacks,
// BMFont, built-in) at one size, shaped by the same pipeline as world text:
// each atlas pixel covers `texel` x `texel` output pixels and glyphs start on
// whole output pixels, so they are drawn without resampling.
class TextFace {
 public:
  // density: output pixels per UI pixel.
  TextFace(core::FontSourcePtr font, int size, float density);

  const Rml::FontMetrics& metrics() const { return metrics_; }
  int width(Rml::StringView text, Rml::Style::Direction dir);
  // One line at `baseline` (its left end); returns its width in UI pixels.
  int generate(Rml::RenderManager& render, Rml::StringView text, Rml::Vector2f baseline, Rml::ColourbPremultiplied colour,
               Rml::Style::Direction dir, Rml::TexturedMeshList& meshes);
  // Changes whenever the atlas image does (RmlUi then generates its text again).
  int version() const { return version_; }

 private:
  core::ShapedText shape(Rml::StringView text, Rml::Style::Direction dir) const;
  Rml::CallbackTextureSource& texture_of(const core::ImagePtr& image);

  struct Texture {
    core::ImagePtr image;
    Rml::CallbackTextureSource source;
  };
  core::FontSourcePtr font_;
  core::FontScale scale_;
  float unit_;     // UI pixels per atlas pixel
  float density_;  // output pixels per UI pixel
  Rml::FontMetrics metrics_{};
  std::deque<Texture> textures_;  // newest last; a few kept for text made before a change
  int version_ = 1;
};

}  // namespace alpha::ui
