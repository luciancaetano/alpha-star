#include "ui/text_face.hpp"

#include <RmlUi/Core/RenderManager.h>

#include <cmath>
#include <string_view>
#include <utility>
#include <vector>

namespace alpha::ui {

namespace {

constexpr std::size_t kTextures = 3;  // atlas images kept per face

// The atlas as premultiplied RGBA (RmlUi composites premultiplied).
std::vector<Rml::byte> premultiplied(const core::Image& image) {
  std::vector<Rml::byte> out(image.rgba.begin(), image.rgba.end());
  for (std::size_t i = 0; i + 3 < out.size(); i += 4) {
    for (std::size_t c = 0; c < 3; ++c) out[i + c] = static_cast<Rml::byte>(out[i + c] * out[i + 3] / 255);
  }
  return out;
}

core::TextDir dir_of(Rml::Style::Direction dir) {
  if (dir == Rml::Style::Direction::Ltr) return core::TextDir::Ltr;
  if (dir == Rml::Style::Direction::Rtl) return core::TextDir::Rtl;
  return core::TextDir::Auto;
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

TextFace::TextFace(core::FontSourcePtr font, int size, float density)
    : font_(std::move(font)),
      scale_(core::font_scale(font_->native(), static_cast<float>(size), density)),
      unit_(scale_.texel / density),
      density_(density) {
  const core::FontAtlas& a = *shape({}, Rml::Style::Direction::Auto).atlas;
  metrics_ = {size, a.ascent * unit_, (a.line_height - a.ascent) * unit_, a.line_height * unit_, a.x_height * unit_,
              unit_, unit_, false};
}

core::ShapedText TextFace::shape(Rml::StringView text, Rml::Style::Direction dir) const {
  return font_->shape({std::string_view(text.begin(), text.size()), scale_.pixels, dir_of(dir), 0.0f});
}

int TextFace::width(Rml::StringView text, Rml::Style::Direction dir) {
  return static_cast<int>(std::lround(shape(text, dir).width() * unit_));
}

Rml::CallbackTextureSource& TextFace::texture_of(const core::ImagePtr& image) {
  if (!textures_.empty() && textures_.back().image == image) return textures_.back().source;
  Rml::CallbackTextureSource source([image](const Rml::CallbackTextureInterface& out) {
    const std::vector<Rml::byte> pixels = premultiplied(*image);
    return out.GenerateTexture({pixels.data(), pixels.size()}, {image->width, image->height});
  });
  textures_.push_back({image, std::move(source)});
  if (textures_.size() > kTextures) textures_.pop_front();
  ++version_;
  return textures_.back().source;
}

int TextFace::generate(Rml::RenderManager& render, Rml::StringView text, Rml::Vector2f baseline,
                       Rml::ColourbPremultiplied colour, Rml::Style::Direction dir, Rml::TexturedMeshList& meshes) {
  const core::ShapedText shaped = shape(text, dir);
  const core::TextQuads q = core::layout_text(shaped, core::Align::Center);
  const core::FontAtlas& atlas = *shaped.atlas;
  const Rml::Vector2f top{std::round(baseline.x * density_) / density_,
                          std::round((baseline.y - atlas.ascent * unit_) * density_) / density_};
  const core::Image& image = *atlas.image;
  const Rml::Vector2f texel{1.0f / static_cast<float>(image.width), 1.0f / static_cast<float>(image.height)};
  Rml::TexturedMesh& out = meshes.emplace_back();
  out.texture = texture_of(atlas.image).GetTexture(render);
  for (const core::TileQuad& t : q.grid.tiles) {
    const Rml::Vector2f a{top.x + t.pos.x * unit_, top.y + t.pos.y * unit_};
    const Rml::Vector2f size{static_cast<float>(t.src.w), static_cast<float>(t.src.h)};
    const Rml::Vector2f uv{static_cast<float>(t.src.x) * texel.x, static_cast<float>(t.src.y) * texel.y};
    quad(out.mesh, {a, a + size * unit_, uv, uv + Rml::Vector2f{size.x * texel.x, size.y * texel.y}}, colour);
  }
  return static_cast<int>(std::lround(q.box.x * unit_));
}

}  // namespace alpha::ui
