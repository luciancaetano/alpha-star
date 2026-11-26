#pragma once

#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/StringUtilities.h>

#include <map>
#include <memory>
#include <string>
#include <tuple>

#include "core/glyph_layout.hpp"

namespace alpha::ui {

class TextFace;

// The game UI's font engine: every assets.fonts font (TrueType/OpenType with
// its fallbacks, BMFont, built-in) is shaped and drawn by the same font
// sources as world text (platform/fonts: HarfBuzz, bidi, fallback, boxes for
// missing glyphs), through TextFace. UI pixels are the game's logical pixels,
// but glyphs are made at the output resolution (`density` output pixels per
// UI pixel) and placed on whole output pixels. RCSS `direction` sets the
// paragraph direction. With no font loaded at all, every family falls back
// to the built-in font. GenerateString's nine parameters are RmlUi's
// interface (tools/harness_config.py EXTERNAL_SIGNATURES).
class FontEngine final : public Rml::FontEngineInterface {
 public:
  FontEngine();
  ~FontEngine() override;

  void add_font(const std::string& family, core::FontSourcePtr font);
  // True when it changed; then every face must be made again (Rml::ReleaseFontResources).
  bool set_density(float density);

  void Shutdown() override;
  Rml::FontFaceHandle GetFontFaceHandle(const Rml::String& family, Rml::Style::FontStyle style,
                                        Rml::Style::FontWeight weight, int size) override;
  Rml::FontEffectsHandle PrepareFontEffects(Rml::FontFaceHandle handle, const Rml::FontEffectList& font_effects) override;
  const Rml::FontMetrics& GetFontMetrics(Rml::FontFaceHandle handle) override;
  int GetStringWidth(Rml::FontFaceHandle handle, Rml::StringView string, const Rml::TextShapingContext& shaping,
                     Rml::Character prior_character) override;
  int GenerateString(Rml::RenderManager& render_manager, Rml::FontFaceHandle face_handle,
                     Rml::FontEffectsHandle effects_handle, Rml::StringView string, Rml::Vector2f position,
                     Rml::ColourbPremultiplied colour, float opacity, const Rml::TextShapingContext& shaping,
                     Rml::TexturedMeshList& mesh_list) override;
  int GetVersion(Rml::FontFaceHandle handle) override;
  void ReleaseFontResources() override;

 private:
  using Key = std::pair<std::string, int>;

  float density_ = 1.0f;
  std::map<std::string, core::FontSourcePtr> fonts_;
  std::map<Key, std::unique_ptr<TextFace>> faces_;
};

}  // namespace alpha::ui
