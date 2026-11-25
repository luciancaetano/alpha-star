#include "ui/rml_renderer.hpp"

#include <RmlUi/Core/Log.h>

#include <cstdint>
#include <exception>

#include "platform/assets.hpp"

namespace alpha::ui {

namespace {

// Compiled geometry: vertices already in SDL form, drawn with a translation.
struct Mesh {
  std::vector<SDL_Vertex> vertices;
  std::vector<int> indices;
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
  for (SDL_Vertex& v : scratch_) {
    v.position.x += translation.x;
    v.position.y += translation.y;
  }
  SDL_RenderGeometry(sdl_, reinterpret_cast<SDL_Texture*>(texture), scratch_.data(),
                     static_cast<int>(scratch_.size()), mesh->indices.data(),
                     static_cast<int>(mesh->indices.size()));
}

void RmlRenderer::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
  delete reinterpret_cast<Mesh*>(geometry);
}

SDL_Texture* RmlRenderer::upload(const Rml::byte* rgba, Rml::Vector2i dimensions) {
  SDL_Texture* tex = SDL_CreateTexture(sdl_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                                       dimensions.x, dimensions.y);
  if (!tex) return nullptr;
  SDL_UpdateTexture(tex, nullptr, rgba, dimensions.x * 4);
  SDL_SetTextureBlendMode(tex, premultiplied_);
  return tex;
}

// `source` is already resolved by RmlUi against the document's own path.
Rml::TextureHandle RmlRenderer::LoadTexture(Rml::Vector2i& dimensions, const Rml::String& source) {
  if (!sdl_) return {};
  try {
    core::Image image = platform::load_image(source);
    premultiply(image.rgba);
    dimensions = {image.width, image.height};
    return reinterpret_cast<Rml::TextureHandle>(upload(image.rgba.data(), dimensions));
  } catch (const std::exception& e) {
    Rml::Log::Message(Rml::Log::LT_WARNING, "%s", e.what());
    return {};
  }
}

Rml::TextureHandle RmlRenderer::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions) {
  if (!sdl_) return {};
  return reinterpret_cast<Rml::TextureHandle>(upload(source.data(), dimensions));
}

void RmlRenderer::ReleaseTexture(Rml::TextureHandle texture) {
  SDL_DestroyTexture(reinterpret_cast<SDL_Texture*>(texture));
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
