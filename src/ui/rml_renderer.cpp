#include "ui/rml_renderer.hpp"

#include <RmlUi/Core/Log.h>

#include <cstdint>
#include <exception>

#include "core/image_filter.hpp"
#include "platform/assets.hpp"

namespace alpha::ui {

namespace {

// Compiled geometry: vertices already in SDL form, drawn with a translation.
struct Mesh {
  std::vector<SDL_Vertex> vertices;
  std::vector<int> indices;
};

// A texture: premultiplied pixels kept on the CPU, uploaded on first draw
// (sharp: already grown, so `size` is the uploaded size).
// RmlUi loads textures during layout, which also happens headless or before
// the window exists; a handle without pixels would be cached as a failure.
struct Texture {
  Rml::Vector2i size;
  std::vector<Rml::byte> rgba;
  SDL_Texture* uploaded = nullptr;
  bool nearest = false;  // glyph atlases and nearest images; else linear
};

SDL_Vertex to_sdl(const Rml::Vertex& v) {
  constexpr float k = 1.0f / 255.0f;
  const Rml::ColourbPremultiplied& c = v.colour;
  return {{v.position.x, v.position.y}, {c.red * k, c.green * k, c.blue * k, c.alpha * k},
          {v.tex_coord.x, v.tex_coord.y}};
}

// RmlUi composites with premultiplied alpha.
void premultiply(std::vector<std::uint8_t>& rgba) {
  for (std::size_t i = 0; i + 3 < rgba.size(); i += 4) {
    for (std::size_t c = 0; c < 3; ++c) {
      rgba[i + c] = static_cast<std::uint8_t>(rgba[i + c] * rgba[i + 3] / 255);
    }
  }
}

}  // namespace

RmlRenderer::RmlRenderer()
    : premultiplied_(SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                                                SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE,
                                                SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD)) {}

void RmlRenderer::begin() {
  SDL_GetRenderDrawBlendMode(sdl_, &previous_);
  SDL_SetRenderDrawBlendMode(sdl_, premultiplied_);
}

void RmlRenderer::end() {
  SDL_SetRenderClipRect(sdl_, nullptr);
  scissor_on_ = false;
  SDL_SetRenderDrawBlendMode(sdl_, previous_);
}

Rml::CompiledGeometryHandle RmlRenderer::CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                         Rml::Span<const int> indices) {
  auto* mesh = new Mesh{{}, {indices.begin(), indices.end()}};
  mesh->vertices.reserve(vertices.size());
  for (const Rml::Vertex& v : vertices) mesh->vertices.push_back(to_sdl(v));
  return reinterpret_cast<Rml::CompiledGeometryHandle>(mesh);
}

void RmlRenderer::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
                                 Rml::TextureHandle texture) {
  if (!sdl_) return;
  const auto* mesh = reinterpret_cast<const Mesh*>(geometry);
  scratch_ = mesh->vertices;
  const core::Vec2 at = core::snap(grid_, {translation.x, translation.y});
  for (SDL_Vertex& v : scratch_) {
    v.position.x += at.x;
    v.position.y += at.y;
  }
  SDL_RenderGeometry(sdl_, uploaded(texture), scratch_.data(),
                     static_cast<int>(scratch_.size()), mesh->indices.data(),
                     static_cast<int>(mesh->indices.size()));
}

void RmlRenderer::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
  delete reinterpret_cast<Mesh*>(geometry);
}

SDL_Texture* RmlRenderer::uploaded(Rml::TextureHandle handle) {
  auto* tex = reinterpret_cast<Texture*>(handle);
  if (!tex || tex->uploaded) return tex ? tex->uploaded : nullptr;
  tex->uploaded = SDL_CreateTexture(sdl_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, tex->size.x, tex->size.y);
  SDL_UpdateTexture(tex->uploaded, nullptr, tex->rgba.data(), tex->size.x * 4);
  SDL_SetTextureBlendMode(tex->uploaded, premultiplied_);
  if (tex->nearest) SDL_SetTextureScaleMode(tex->uploaded, SDL_SCALEMODE_NEAREST);
  return tex->uploaded;
}

// `source` is already resolved by RmlUi against the document's own path.
Rml::TextureHandle RmlRenderer::LoadTexture(Rml::Vector2i& dimensions, const Rml::String& source) {
  try {
    core::Image image = platform::load_image(source);
    premultiply(image.rgba);
    dimensions = {image.width, image.height};
    // Sharp: grown by a whole factor, drawn linear; tex coords are normalized.
    if (filter_ == core::Filter::sharp) image = core::upscaled(image, core::sharp_factor(image.width, image.height));
    return reinterpret_cast<Rml::TextureHandle>(
        new Texture{{image.width, image.height}, std::move(image.rgba), nullptr, filter_ == core::Filter::nearest});
  } catch (const std::exception& e) {
    Rml::Log::Message(Rml::Log::LT_WARNING, "%s", e.what());
    return {};
  }
}

Rml::TextureHandle RmlRenderer::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions) {
  return reinterpret_cast<Rml::TextureHandle>(new Texture{dimensions, {source.begin(), source.end()}, nullptr, true});
}

// Uploaded textures belong to the attached renderer (Ui::attach releases all
// textures before switching), so they are destroyed while it still exists.
void RmlRenderer::ReleaseTexture(Rml::TextureHandle texture) {
  auto* tex = reinterpret_cast<Texture*>(texture);
  if (tex->uploaded) SDL_DestroyTexture(tex->uploaded);
  delete tex;
}

void RmlRenderer::EnableScissorRegion(bool enable) {
  scissor_on_ = enable;
  if (sdl_) SDL_SetRenderClipRect(sdl_, enable ? &scissor_ : nullptr);
}

void RmlRenderer::SetScissorRegion(Rml::Rectanglei region) {
  scissor_ = {region.Left(), region.Top(), region.Width(), region.Height()};
  if (sdl_ && scissor_on_) SDL_SetRenderClipRect(sdl_, &scissor_);
}

}  // namespace alpha::ui
