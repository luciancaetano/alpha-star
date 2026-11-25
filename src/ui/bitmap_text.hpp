#pragma once

#include <RmlUi/Core/CallbackTexture.h>
#include <RmlUi/Core/FontMetrics.h>
#include <RmlUi/Core/Mesh.h>
#include <RmlUi/Core/StringUtilities.h>
#include <RmlUi/Core/Types.h>

#include "core/glyph_layout.hpp"

namespace Rml {
class RenderManager;
}

namespace alpha::ui {

// Game UI text in a bitmap font (BMFont or the built-in one) at one size:
// each atlas pixel covers `texel` x `texel` output pixels and glyphs start on
// whole output pixels, so they are drawn without resampling.
class BitmapText {
 public:
  // density: output pixels per UI pixel.
  BitmapText(core::FontAtlasPtr atlas, core::FontScale scale, float density);

  Rml::FontMetrics metrics(int size) const;
  int width(Rml::StringView text) const;
  // One line at `baseline` (its left end); returns its width in UI pixels.
  int generate(Rml::RenderManager& render, Rml::StringView text, Rml::Vector2f baseline,
               Rml::ColourbPremultiplied colour, Rml::TexturedMeshList& meshes);

 private:
  core::FontAtlasPtr atlas_;
  float unit_;     // UI pixels per atlas pixel
  float density_;  // output pixels per UI pixel
  Rml::CallbackTextureSource texture_;
};

}  // namespace alpha::ui
