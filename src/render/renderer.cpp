#include "render/renderer.hpp"

#include <cmath>
#include <utility>

#include "core/particles.hpp"
#include "platform/vfs.hpp"
#include "render/quads.hpp"
#include "render/text_layout.hpp"

namespace alpha::render {

namespace {

SDL_FColor to_sdl(core::Color c) {
  constexpr float k = 1.0f / 255.0f;
  return {c.r * k, c.g * k, c.b * k, c.a * k};
}

SDL_BlendMode sdl_blend(core::Blend b) {
  if (b == core::Blend::Add) return SDL_BLENDMODE_ADD;
  if (b == core::Blend::Multiply) return SDL_BLENDMODE_MUL;
  return SDL_BLENDMODE_BLEND;
}

SDL_RendererLogicalPresentation sdl_presentation(core::Scaling s) {
  if (s == core::Scaling::Integer) return SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
  if (s == core::Scaling::Stretch) return SDL_LOGICAL_PRESENTATION_STRETCH;
  return SDL_LOGICAL_PRESENTATION_LETTERBOX;  // expand: letterbox of a size with the window's aspect
}

}  // namespace

Frame::Frame(core::Color bg, std::vector<core::DrawItem> s, std::vector<core::TextItem> t)
    : background(bg), shapes(std::move(s)), texts(std::move(t)) {}

Renderer::Renderer(SDL_Renderer* sdl, int width, int height, core::Scaling scaling)
    : sdl_(sdl),
      game_size_{static_cast<float>(width), static_cast<float>(height)},
      size_(game_size_),
      scaling_(scaling) {
  present_logical();
  SDL_SetRenderDrawBlendMode(sdl_, SDL_BLENDMODE_BLEND);
}

// expand: the logical size takes the output's aspect (whole pixels).
void Renderer::present_logical() {
  if (scaling_ == core::Scaling::Expand) {
    int w = 0;
    int h = 0;
    SDL_GetRenderOutputSize(sdl_, &w, &h);
    const core::Vec2 grown = core::expanded_size(game_size_, {static_cast<float>(w), static_cast<float>(h)});
    size_ = {std::round(grown.x), std::round(grown.y)};
  }
  SDL_SetRenderLogicalPresentation(sdl_, static_cast<int>(size_.x), static_cast<int>(size_.y), sdl_presentation(scaling_));
}

// Output pixels per game pixel (logical presentation), so feathers stay 1 output pixel wide.
float Renderer::output_scale() const {
  SDL_FRect shown{};
  if (!SDL_GetRenderLogicalPresentationRect(sdl_, &shown) || shown.w <= 0) return 1.0f;
  return shown.w / size_.x;
}

Renderer::~Renderer() {
  for (const auto& [image, tex] : textures_) SDL_DestroyTexture(tex);
}

void Renderer::draw(const Frame& frame) {
  if (scaling_ == core::Scaling::Expand) present_logical();
  const core::Color bg = frame.background;
  SDL_SetRenderDrawColor(sdl_, bg.r, bg.g, bg.b, bg.a);
  SDL_RenderClear(sdl_);
  stats_ = {};
  window_scale_ = output_scale();
  core::View view = frame.view.value_or(core::View{size_ * 0.5f, 1.0f, 0.0f, size_});
  view.size = size_;
  draw_shapes(frame.shapes, view);
  for (const core::TextItem& text : frame.texts) draw_text(text);
}

// Items go out in order as batches of consecutive items sharing a key
// (plain shapes share "no texture"); items off screen are skipped (particles
// and tiles per particle / per cell).
void Renderer::draw_shapes(const std::vector<core::DrawItem>& shapes, const core::View& view) {
  batch_.clear();
  batch_key_ = {};
  for (const core::DrawItem& item : shapes) add(item, view);
  flush(view);
  SDL_SetRenderDrawBlendMode(sdl_, SDL_BLENDMODE_BLEND);
}

void Renderer::add(const core::DrawItem& item, const core::View& view) {
  const core::Box area = core::visible_area(core::parallax_view(view, item.parallax));
  if (item.particles.empty() && !core::overlaps(bounds_of(item), area)) {
    ++stats_.culled;
    return;
  }
  const core::ShapeSpec& shape = item.shape;
  if (shape.text && !shape.image) return draw_world_text(item, view);
  const BatchKey key{shape.image ? texture(shape.image) : nullptr, shape.blend, item.parallax};
  if (!(key == batch_key_)) {
    flush(view);
    batch_key_ = key;
  }
  if (!item.particles.empty()) particle_triangles(item, area, batch_);
  else if (key.texture) image_triangles(item, area, batch_);
  else tessellate(item, batch_, 1.0f / (core::parallax_view(view, item.parallax).zoom * window_scale_));
  ++stats_.drawn;
}

// The batch through its layer's camera, as one SDL_RenderGeometry call.
void Renderer::flush(const core::View& view) {
  if (batch_.empty()) return;
  const core::View seen = core::parallax_view(view, batch_key_.parallax);
  const SDL_BlendMode mode = sdl_blend(batch_key_.blend);
  if (batch_key_.texture) SDL_SetTextureBlendMode(batch_key_.texture, mode);
  else SDL_SetRenderDrawBlendMode(sdl_, mode);
  vertices_.clear();
  for (const Vertex& v : batch_) {
    const core::Vec2 p = core::to_screen(seen, v.pos);
    vertices_.push_back({{p.x, p.y}, to_sdl(v.color), {v.uv.x, v.uv.y}});
  }
  SDL_RenderGeometry(sdl_, batch_key_.texture, vertices_.data(), static_cast<int>(vertices_.size()), nullptr, 0);
  ++stats_.draw_calls;
  batch_.clear();
}

SDL_Texture* Renderer::texture(const core::ImagePtr& image) {
  if (const auto it = textures_.find(image.get()); it != textures_.end()) return it->second;
  SDL_Texture* tex = SDL_CreateTexture(sdl_, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                                       image->width, image->height);
  SDL_UpdateTexture(tex, nullptr, image->rgba.data(), image->width * 4);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(tex, image->smooth ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);  // pixelMode
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

// Text without a font: the debug font, placed and scaled through the camera
// (not rotated), in z order with everything else.
void Renderer::draw_world_text(const core::DrawItem& item, const core::View& view) {
  flush(view);
  const core::View seen = core::parallax_view(view, item.parallax);
  const core::TextLook& t = *item.shape.text;
  const core::Color c = core::shade(item.shape.fill, item.shape.tint, item.shape.alpha);
  SDL_SetRenderDrawBlendMode(sdl_, sdl_blend(item.shape.blend));
  draw_text({t.text, core::to_screen(seen, item.pos), t.size * seen.zoom, c, t.align});
  ++stats_.drawn;
}

void Renderer::present() { SDL_RenderPresent(sdl_); }

bool Renderer::save_bmp(const std::string& path) const {
  SDL_Surface* pixels = SDL_RenderReadPixels(sdl_, nullptr);
  const auto bmp = platform::vfs::written([pixels](SDL_IOStream* io) { return pixels && SDL_SaveBMP_IO(pixels, io, false); });
  SDL_DestroySurface(pixels);
  try {
    if (bmp) platform::vfs::write_atomic(path, *bmp, platform::vfs::Backup::none);
    return bmp.has_value();
  } catch (const std::exception&) {
    return false;
  }
}

}  // namespace alpha::render
