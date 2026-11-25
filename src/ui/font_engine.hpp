#pragma once

#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/StringUtilities.h>

#include <map>
#include <memory>
#include <string>
#include <tuple>

#include "core/glyph_layout.hpp"

namespace alpha::ui {

class BitmapText;

// The game UI's font engine. UI pixels are the game's logical pixels, but
// glyphs are made at the output resolution (`density` output pixels per UI
// pixel) and placed on whole output pixels, so the logical presentation never
// resamples text. TrueType/OpenType goes to RmlUi's own FreeType engine
// (`inner`) at size x density, its geometry scaled back; BMFont and the
// built-in font are drawn here (BitmapText). With no font loaded at all, every
// family falls back to the built-in font. GenerateString's nine parameters are
// RmlUi's interface (tools/harness_config.py EXTERNAL_SIGNATURES).
class FontEngine final : public Rml::FontEngineInterface {
 public:
  explicit FontEngine(Rml::FontEngineInterface& inner);
  ~FontEngine() override;

  void add_bitmap(const std::string& family, core::FontSourcePtr font);
  // True when it changed; then every face must be made again (Rml::ReleaseFontResources).
  bool set_density(float density);

  // TrueType / OpenType: registered with RmlUi's engine directly (Library).
  Rml::FontEngineInterface& inner() { return inner_; }
  void loaded_font() { any_font_ = true; }

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
  // A family at one size: RmlUi's face at size x density, or bitmap text.
  struct Face {
    Rml::FontFaceHandle inner = 0;
    std::unique_ptr<BitmapText> bitmap;
    Rml::FontMetrics metrics{};
  };
  using Key = std::tuple<std::string, int, int, int>;

  Face* make_face(const Rml::String& family, Rml::Style::FontStyle style, Rml::Style::FontWeight weight, int size);
  Face* bitmap_face(const core::FontSourcePtr& font, int size);
  Rml::TextShapingContext scaled(const Rml::TextShapingContext& shaping) const;
  int to_ui(int output_pixels) const;

  Rml::FontEngineInterface& inner_;
  float density_ = 1.0f;
  bool any_font_ = false;
  std::map<std::string, core::FontSourcePtr> bitmaps_;
  std::map<Key, std::unique_ptr<Face>> faces_;
};

}  // namespace alpha::ui
