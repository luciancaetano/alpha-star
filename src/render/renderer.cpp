#include "render/renderer.hpp"

#include "render/text_layout.hpp"

namespace alpha::render {

namespace {

SDL_FColor to_sdl(core::Color c) {
  constexpr float k = 1.0f / 255.0f;
  return {c.r * k, c.g * k, c.b * k, c.a * k};
}

}  // namespace

Renderer::Renderer(SDL_Renderer* sdl, int width, int height) : sdl_(sdl) {
  SDL_SetRenderLogicalPresentation(sdl_, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_SetRenderDrawBlendMode(sdl_, SDL_BLENDMODE_BLEND);
}

Renderer::~Renderer() {
  for (const auto& [image, tex] : textures_) SDL_DestroyTexture(tex);
}

void Renderer::draw(const Frame& frame) {
  const core::Color bg = frame.background;
  SDL_SetRenderDrawColor(sdl_, bg.r, bg.g, bg.b, bg.a);
  SDL_RenderClear(sdl_);
  draw_shapes(frame.shapes);
  for (const core::TextItem& text : frame.texts) draw_text(text);
}

// Consecutive plain shapes go out as one geometry batch; images break the
// batch so z order holds.
void Renderer::draw_shapes(const std::vector<core::DrawItem>& shapes) {
  const std::span<const core::DrawItem> all(shapes);
  std::size_t start = 0;
  for (std::size_t i = 0; i < all.size(); ++i) {
    if (!all[i].shape.image) continue;
    draw_geometry(all.subspan(start, i - start));
    draw_image(all[i]);
    start = i + 1;
  }
  draw_geometry(all.subspan(start));
}

SDL_Texture* Renderer::texture(const core::ImagePtr& image) {
  if (const auto it = textures_.find(image.get()); it != textures_.end()) return it->second;
  SDL_Texture* tex = SDL_CreateTexture(sdl_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                                       image->width, image->height);
  SDL_UpdateTexture(tex, nullptr, image->rgba.data(), image->width * 4);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);  // crisp pixel art
  textures_[image.get()] = tex;
  return tex;
}

void Renderer::draw_image(const core::DrawItem& item) {
  const core::Vec2 size = std::get<core::Rect>(item.shape.geometry).size;
  const SDL_FRect dst{item.pos.x - size.x / 2, item.pos.y - size.y / 2, size.x, size.y};
  SDL_RenderTextureRotated(sdl_, texture(item.shape.image), nullptr, &dst, item.rotation, nullptr,
                           SDL_FLIP_NONE);
}

void Renderer::draw_geometry(std::span<const core::DrawItem> items) {
  if (items.empty()) return;
  vertices_.clear();
  for (const Vertex& v : tessellate(items)) {
    vertices_.push_back({{v.pos.x, v.pos.y}, to_sdl(v.color), {0.0f, 0.0f}});
  }
  SDL_RenderGeometry(sdl_, nullptr, vertices_.data(), static_cast<int>(vertices_.size()),
                     nullptr, 0);
}

void Renderer::draw_text(const core::TextItem& text) {
  const TextPlacement at = place_text(text);
  SDL_SetRenderDrawColor(sdl_, text.color.r, text.color.g, text.color.b, text.color.a);
  SDL_SetRenderScale(sdl_, at.scale, at.scale);
  SDL_RenderDebugText(sdl_, at.top_left.x, at.top_left.y, text.text.c_str());
  SDL_SetRenderScale(sdl_, 1.0f, 1.0f);
}

void Renderer::present() { SDL_RenderPresent(sdl_); }

bool Renderer::save_bmp(const std::string& path) const {
  SDL_Surface* pixels = SDL_RenderReadPixels(sdl_, nullptr);
  const bool ok = pixels && SDL_SaveBMP(pixels, path.c_str());
  SDL_DestroySurface(pixels);
  return ok;
}

}  // namespace alpha::render
