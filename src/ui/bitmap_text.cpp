#include "ui/bitmap_text.hpp"

#include <RmlUi/Core/RenderManager.h>

#include <cmath>
#include <string_view>
#include <utility>
#include <vector>

namespace alpha::ui {

namespace {

// The atlas as premultiplied RGBA (RmlUi composites premultiplied).
std::vector<Rml::byte> premultiplied(const core::Image& image) {
  std::vector<Rml::byte> out(image.rgba.begin(), image.rgba.end());
  for (std::size_t i = 0; i + 3 < out.size(); i += 4) {
    for (std::size_t c = 0; c < 3; ++c) out[i + c] = static_cast<Rml::byte>(out[i + c] * out[i + 3] / 255);
  }
  return out;
}

// Corners a (top-left) and b of a quad on screen and in the texture.
struct Corners {
  Rml::Vector2f a;
  Rml::Vector2f b;
  Rml::Vector2f uv_a;
  Rml::Vector2f uv_b;
};

void quad(Rml::Mesh& mesh, const Corners& q, Rml::ColourbPremultiplied colour) {
  const auto first = static_cast<int>(mesh.vertices.size());
  mesh.vertices.push_back({q.a, colour, q.uv_a});
  mesh.vertices.push_back({{q.b.x, q.a.y}, colour, {q.uv_b.x, q.uv_a.y}});
  mesh.vertices.push_back({q.b, colour, q.uv_b});
  mesh.vertices.push_back({{q.a.x, q.b.y}, colour, {q.uv_a.x, q.uv_b.y}});
  for (const int i : {0, 1, 2, 0, 2, 3}) mesh.indices.push_back(first + i);
}

}  // namespace

BitmapText::BitmapText(core::FontAtlasPtr atlas, core::FontScale scale, float density)
    : atlas_(std::move(atlas)), unit_(scale.texel / density), density_(density) {
  texture_ = Rml::CallbackTextureSource([image = atlas_->image](const Rml::CallbackTextureInterface& out) {
    const std::vector<Rml::byte> pixels = premultiplied(*image);
    return out.GenerateTexture({pixels.data(), pixels.size()}, {image->width, image->height});
  });
}

Rml::FontMetrics BitmapText::metrics(int size) const {
  const core::Glyph& x = atlas_->glyphs[static_cast<std::size_t>(U'x' - U' ')];
  const float ascent = atlas_->ascent * unit_;
  const float x_height = x.src.h > 0 ? static_cast<float>(x.src.h) * unit_ : ascent * 0.5f;
  return {size, ascent, (atlas_->line_height - atlas_->ascent) * unit_, atlas_->line_height * unit_, x_height, unit_, unit_,
          false};
}

int BitmapText::width(Rml::StringView text) const {
  const core::TextQuads q = core::layout_text(*atlas_, std::string_view(text.begin(), text.size()), core::Align::Center);
  return static_cast<int>(std::lround(q.box.x * unit_));
}

int BitmapText::generate(Rml::RenderManager& render, Rml::StringView text, Rml::Vector2f baseline,
                         Rml::ColourbPremultiplied colour, Rml::TexturedMeshList& meshes) {
  const core::TextQuads q = core::layout_text(*atlas_, std::string_view(text.begin(), text.size()), core::Align::Center);
  const Rml::Vector2f top{std::round(baseline.x * density_) / density_,
                          std::round((baseline.y - atlas_->ascent * unit_) * density_) / density_};
  const core::Image& image = *atlas_->image;
  const Rml::Vector2f texel{1.0f / static_cast<float>(image.width), 1.0f / static_cast<float>(image.height)};
  Rml::TexturedMesh& out = meshes.emplace_back();
  out.texture = texture_.GetTexture(render);
  for (const core::TileQuad& t : q.grid.tiles) {
    const Rml::Vector2f a{top.x + t.pos.x * unit_, top.y + t.pos.y * unit_};
    const Rml::Vector2f size{static_cast<float>(t.src.w), static_cast<float>(t.src.h)};
    const Rml::Vector2f uv{static_cast<float>(t.src.x) * texel.x, static_cast<float>(t.src.y) * texel.y};
    quad(out.mesh, {a, a + size * unit_, uv, uv + Rml::Vector2f{size.x * texel.x, size.y * texel.y}}, colour);
  }
  return static_cast<int>(std::lround(q.box.x * unit_));
}

}  // namespace alpha::ui
