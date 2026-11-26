#include "ui/font_engine.hpp"

#include <utility>

#include "platform/fonts.hpp"
#include "ui/text_face.hpp"

namespace alpha::ui {

namespace {

TextFace& face_of(Rml::FontFaceHandle handle) { return *reinterpret_cast<TextFace*>(handle); }

}  // namespace

FontEngine::FontEngine() = default;

FontEngine::~FontEngine() = default;

void FontEngine::add_font(const std::string& family, core::FontSourcePtr font) {
  fonts_[family] = std::move(font);  // faces made before keep their font until ReleaseFontResources
}

bool FontEngine::set_density(float density) {
  if (!(density > 0.0f) || density == density_) return false;
  density_ = density;
  return true;
}

void FontEngine::Shutdown() {
  faces_.clear();
  fonts_.clear();
}

// Weight and style are not told apart: load each style as its own family.
Rml::FontFaceHandle FontEngine::GetFontFaceHandle(const Rml::String& family, Rml::Style::FontStyle,
                                                  Rml::Style::FontWeight, int size) {
  core::FontSourcePtr font;
  if (const auto it = fonts_.find(family); it != fonts_.end()) font = it->second;
  else if (fonts_.empty()) font = platform::default_font();
  if (!font) return 0;
  auto& face = faces_[{family, size}];
  if (!face) face = std::make_unique<TextFace>(std::move(font), size, density_);
  return reinterpret_cast<Rml::FontFaceHandle>(face.get());
}

Rml::FontEffectsHandle FontEngine::PrepareFontEffects(Rml::FontFaceHandle, const Rml::FontEffectList&) { return 0; }

const Rml::FontMetrics& FontEngine::GetFontMetrics(Rml::FontFaceHandle handle) { return face_of(handle).metrics(); }

int FontEngine::GetStringWidth(Rml::FontFaceHandle handle, Rml::StringView string, const Rml::TextShapingContext& shaping,
                               Rml::Character) {
  return face_of(handle).width(string, shaping.text_direction);
}

int FontEngine::GenerateString(Rml::RenderManager& render_manager, Rml::FontFaceHandle face_handle,
                               Rml::FontEffectsHandle, Rml::StringView string, Rml::Vector2f position,
                               Rml::ColourbPremultiplied colour, float, const Rml::TextShapingContext& shaping,
                               Rml::TexturedMeshList& mesh_list) {
  return face_of(face_handle).generate(render_manager, string, position, colour, shaping.text_direction, mesh_list);
}

int FontEngine::GetVersion(Rml::FontFaceHandle handle) { return face_of(handle).version(); }

void FontEngine::ReleaseFontResources() { faces_.clear(); }

}  // namespace alpha::ui
