#include "render/renderer.hpp"

#include <utility>

#include "render/quads.hpp"
#include "render/text_layout.hpp"

namespace alpha::render {

namespace {

SDL_FColor to_sdl(core::Color c) {
  constexpr float k = 1.0f / 255.0f;
  return {c.r * k, c.g * k, c.b * k, c.a * k};
}

}  // namespace

Frame::Frame(core::Color bg, std::vector<core::DrawItem> s, std::vector<core::TextItem> t)
    : background(bg), shapes(std::move(s)), texts(std::move(t)) {}

Renderer::Renderer(SDL_Renderer* sdl, int width, int height)
    : sdl_(sdl), size_{static_cast<float>(width), static_cast<float>(height)} {
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
  stats_ = {};
  draw_shapes(frame.shapes, frame.view.value_or(core::View{size_ * 0.5f, 1.0f, 0.0f, size_}));
  for (const core::TextItem& text : frame.texts) draw_text(text);
}

// Items off screen are skipped; the rest go out in z order as batches of
// consecutive items sharing a texture (plain shapes share "no texture").
void Renderer::draw_shapes(const std::vector<core::DrawItem>& shapes, const core::View& view) {
  const core::Box area = core::visible_area(view);
  batch_.clear();
  batch_texture_ = nullptr;
  for (const core::DrawItem& item : shapes) {
    if (core::overlaps(bounds_of(item), area)) add(item, area, view);
    else ++stats_.culled;
  }
  flush(view);
}

void Renderer::add(const core::DrawItem& item, const core::Box& area, const core::View& view) {
  SDL_Texture* tex = item.shape.image ? texture(item.shape.image) : nullptr;
  if (tex != batch_texture_) {
    flush(view);
    batch_texture_ = tex;
  }
  if (tex) image_triangles(item, area, batch_);
  else tessellate(item, batch_);
  ++stats_.drawn;
}

// The batch through the camera, as one SDL_RenderGeometry call.
void Renderer::flush(const core::View& view) {
  if (batch_.empty()) return;
  vertices_.clear();
  for (const Vertex& v : batch_) {
    const core::Vec2 p = core::to_screen(view, v.pos);
    vertices_.push_back({{p.x, p.y}, to_sdl(v.color), {v.uv.x, v.uv.y}});
  }
  SDL_RenderGeometry(sdl_, batch_texture_, vertices_.data(), static_cast<int>(vertices_.size()), nullptr, 0);
  ++stats_.draw_calls;
  batch_.clear();
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
