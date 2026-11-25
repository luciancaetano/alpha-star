#include "ui/font_engine.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "platform/fonts.hpp"
#include "ui/bitmap_text.hpp"

namespace alpha::ui {

namespace {

// Moves the vertices RmlUi's engine added (at output pixels) back to UI pixels.
void to_ui_pixels(Rml::TexturedMeshList& meshes, const std::vector<std::size_t>& before, float density) {
  for (std::size_t m = 0; m < meshes.size(); ++m) {
    auto& vertices = meshes[m].mesh.vertices;
    for (std::size_t v = m < before.size() ? before[m] : 0; v < vertices.size(); ++v) vertices[v].position /= density;
  }
}

}  // namespace

FontEngine::FontEngine(Rml::FontEngineInterface& inner) : inner_(inner) {}

FontEngine::~FontEngine() = default;

void FontEngine::add_bitmap(const std::string& family, core::FontSourcePtr font) {
  bitmaps_[family] = std::move(font);
  any_font_ = true;
}

bool FontEngine::set_density(float density) {
  if (!(density > 0.0f) || density == density_) return false;
  density_ = density;
  return true;
}

void FontEngine::Shutdown() {
  faces_.clear();
  bitmaps_.clear();
  inner_.Shutdown();
}

Rml::FontFaceHandle FontEngine::GetFontFaceHandle(const Rml::String& family, Rml::Style::FontStyle style,
                                                  Rml::Style::FontWeight weight, int size) {
  const Key key{family, static_cast<int>(style), static_cast<int>(weight), size};
  auto& face = faces_[key];
  if (!face) face.reset(make_face(family, style, weight, size));
  return reinterpret_cast<Rml::FontFaceHandle>(face.get());
}

FontEngine::Face* FontEngine::make_face(const Rml::String& family, Rml::Style::FontStyle style,
                                        Rml::Style::FontWeight weight, int size) {
  if (const auto it = bitmaps_.find(family); it != bitmaps_.end()) return bitmap_face(it->second, size);
  const auto pixels = static_cast<int>(std::lround(static_cast<float>(size) * density_));
  const Rml::FontFaceHandle inner = inner_.GetFontFaceHandle(family, style, weight, std::max(1, pixels));
  if (!inner) return any_font_ ? nullptr : bitmap_face(platform::default_font(), size);
  auto* face = new Face{inner, nullptr, inner_.GetFontMetrics(inner)};
  Rml::FontMetrics& m = face->metrics;
  m.size = size;
  for (float* v : {&m.ascent, &m.descent, &m.line_spacing, &m.x_height, &m.underline_position, &m.underline_thickness}) {
    *v /= density_;
  }
  return face;
}

FontEngine::Face* FontEngine::bitmap_face(const core::FontSourcePtr& font, int size) {
  const core::FontScale scale = core::font_scale(font->native(), static_cast<float>(size), density_);
  auto* face = new Face{0, std::make_unique<BitmapText>(font->atlas(scale.pixels), scale, density_), {}};
  face->metrics = face->bitmap->metrics(size);
  return face;
}

Rml::TextShapingContext FontEngine::scaled(const Rml::TextShapingContext& shaping) const {
  return {shaping.language, shaping.text_direction, shaping.font_kerning, shaping.letter_spacing * density_};
}

int FontEngine::to_ui(int output_pixels) const {
  return static_cast<int>(std::lround(static_cast<float>(output_pixels) / density_));
}

Rml::FontEffectsHandle FontEngine::PrepareFontEffects(Rml::FontFaceHandle handle, const Rml::FontEffectList& font_effects) {
  const auto* face = reinterpret_cast<const Face*>(handle);
  return face->inner ? inner_.PrepareFontEffects(face->inner, font_effects) : 0;
}

const Rml::FontMetrics& FontEngine::GetFontMetrics(Rml::FontFaceHandle handle) {
  return reinterpret_cast<const Face*>(handle)->metrics;
}

int FontEngine::GetStringWidth(Rml::FontFaceHandle handle, Rml::StringView string, const Rml::TextShapingContext& shaping,
                               Rml::Character prior_character) {
  const auto* face = reinterpret_cast<const Face*>(handle);
  if (face->bitmap) return face->bitmap->width(string);
  return to_ui(inner_.GetStringWidth(face->inner, string, scaled(shaping), prior_character));
}

// TrueType: drawn by RmlUi's engine from a whole output pixel, then scaled
// back, so every glyph corner lands on an output pixel.
int FontEngine::GenerateString(Rml::RenderManager& render_manager, Rml::FontFaceHandle face_handle,
                               Rml::FontEffectsHandle effects_handle, Rml::StringView string, Rml::Vector2f position,
                               Rml::ColourbPremultiplied colour, float opacity, const Rml::TextShapingContext& shaping,
                               Rml::TexturedMeshList& mesh_list) {
  auto* face = reinterpret_cast<Face*>(face_handle);
  if (face->bitmap) return face->bitmap->generate(render_manager, string, position, colour, mesh_list);
  std::vector<std::size_t> before;
  for (const Rml::TexturedMesh& m : mesh_list) before.push_back(m.mesh.vertices.size());
  const Rml::Vector2f at{std::round(position.x * density_), std::round(position.y * density_)};
  const int width = inner_.GenerateString(render_manager, face->inner, effects_handle, string, at, colour, opacity,
                                          scaled(shaping), mesh_list);
  to_ui_pixels(mesh_list, before, density_);
  return to_ui(width);
}

int FontEngine::GetVersion(Rml::FontFaceHandle handle) {
  const auto* face = reinterpret_cast<const Face*>(handle);
  return face->inner ? inner_.GetVersion(face->inner) : 1;
}

void FontEngine::ReleaseFontResources() {
  faces_.clear();
  inner_.ReleaseFontResources();
}

}  // namespace alpha::ui
