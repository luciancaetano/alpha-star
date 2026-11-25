#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <SDL3/SDL.h>

#include <vector>

namespace alpha::ui {

// RmlUi drawing through SDL_Renderer (adapted from RmlUi's SDL backend):
// geometry, textures (images through our image loader, premultiplied) and
// scissor. No transforms, filters or shaders. Coordinates are the game's
// logical pixels: the engine's renderer sets the logical presentation.
// Until a renderer is attached (headless runs) nothing is drawn or uploaded.
class RmlRenderer final : public Rml::RenderInterface {
 public:
  RmlRenderer();
  void attach(SDL_Renderer* sdl) { sdl_ = sdl; }
  SDL_Renderer* attached() const { return sdl_; }
  // Called around Context::Render: premultiplied blending, then restore.
  void begin();
  void end();

  Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                              Rml::Span<const int> indices) override;
  void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
                      Rml::TextureHandle texture) override;
  void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
  Rml::TextureHandle LoadTexture(Rml::Vector2i& dimensions, const Rml::String& source) override;
  Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i dimensions) override;
  void ReleaseTexture(Rml::TextureHandle texture) override;
  void EnableScissorRegion(bool enable) override;
  void SetScissorRegion(Rml::Rectanglei region) override;

 private:
  SDL_Texture* upload(const Rml::byte* rgba, Rml::Vector2i dimensions);

  SDL_Renderer* sdl_ = nullptr;
  SDL_BlendMode premultiplied_;
  SDL_BlendMode previous_ = SDL_BLENDMODE_BLEND;
  SDL_Rect scissor_{};
  bool scissor_on_ = false;
  std::vector<SDL_Vertex> scratch_;
};

}  // namespace alpha::ui
