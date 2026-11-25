#include "render/compositor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "render/light_mesh.hpp"

namespace alpha::render {

namespace {

SDL_FColor to_sdl(core::Color c) {
  constexpr float k = 1.0f / 255.0f;
  return {c.r * k, c.g * k, c.b * k, c.a * k};
}

int whole(float v) { return std::max(1, static_cast<int>(std::ceil(v))); }

std::uint8_t toward_white(std::uint8_t c, float strength) {
  return static_cast<std::uint8_t>(255.0f - (255.0f - c) * strength + 0.5f);
}

}  // namespace

Compositor::~Compositor() {
  for (SDL_Texture* t : targets_) SDL_DestroyTexture(t);
}

// The slot's texture at w x h (made or remade on a size change); null when
// the renderer cannot make render targets.
SDL_Texture* Compositor::target(Slot slot, int w, int h) {
  SDL_Texture*& t = targets_[slot];
  float tw = 0;
  float th = 0;
  if (t && SDL_GetTextureSize(t, &tw, &th) && static_cast<int>(tw) == w && static_cast<int>(th) == h) return t;
  SDL_DestroyTexture(t);
  t = SDL_CreateTexture(sdl_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
  if (!t) unavailable("render targets");
  return t;
}

void Compositor::unavailable(const char* what) {
  if (warned_) return;
  warned_ = true;
  SDL_Log("lighting/postfx: %s unavailable on renderer %s (%s); lighting, bloom and pixelate are skipped", what,
          SDL_GetRendererName(sdl_), SDL_GetError());
}

void Compositor::use(SDL_Texture* t) { SDL_SetRenderTarget(sdl_, t); }

// `from` stretched over all of `to` (null: the window's game area).
void Compositor::blit(SDL_Texture* from, SDL_Texture* to, SDL_BlendMode mode, SDL_ScaleMode scale) {
  SDL_FRect dst{0, 0, size_.x, size_.y};
  if (to) SDL_GetTextureSize(to, &dst.w, &dst.h);
  SDL_SetTextureBlendMode(from, mode);
  SDL_SetTextureScaleMode(from, scale);
  use(to);
  SDL_RenderTexture(sdl_, from, nullptr, &dst);
}

// Sends mesh_ (plain colored triangles) to the current target and clears it.
void Compositor::geometry(SDL_BlendMode mode) {
  vertices_.clear();
  for (const Vertex& v : mesh_) vertices_.push_back({{v.pos.x, v.pos.y}, to_sdl(v.color), {0, 0}});
  SDL_SetRenderDrawBlendMode(sdl_, mode);
  SDL_RenderGeometry(sdl_, nullptr, vertices_.data(), static_cast<int>(vertices_.size()), nullptr, 0);
  SDL_SetRenderDrawBlendMode(sdl_, SDL_BLENDMODE_BLEND);
  mesh_.clear();
}

void Compositor::begin(core::Vec2 size, bool capture) {
  size_ = size;
  canvas_ = capture ? target(kScene, whole(size.x), whole(size.y)) : nullptr;
  use(canvas_);
}

// Lightmap: ambient everywhere, lights added, then multiplied into the canvas.
void Compositor::light(const std::optional<core::Lighting>& lighting, const std::vector<core::LightDisc>& lights) {
  if (!lighting) return;
  SDL_Texture* map = target(kLights, whole(size_.x), whole(size_.y));
  if (!map) return;
  use(map);
  const core::Color a = lighting->ambient;
  SDL_SetRenderDrawColor(sdl_, a.r, a.g, a.b, 255);
  SDL_RenderClear(sdl_);
  for (const core::LightDisc& d : lights) light_triangles(d, mesh_);
  geometry(SDL_BLENDMODE_ADD);
  blit(map, canvas_, SDL_BLENDMODE_MOD, SDL_SCALEMODE_NEAREST);
}

void Compositor::finish(const std::vector<core::Effect>& chain) {
  for (const core::Effect& e : chain) apply(e);
  if (!canvas_) return;
  blit(canvas_, nullptr, SDL_BLENDMODE_NONE, SDL_SCALEMODE_NEAREST);
  canvas_ = nullptr;
}

void Compositor::apply(const core::Effect& e) {
  use(canvas_);
  switch (e.kind) {
    case core::EffectKind::Bloom: return bloom(e);
    case core::EffectKind::Pixelate: return pixelate(e);
    case core::EffectKind::Tint: return tint(e);
    case core::EffectKind::Vignette:
      vignette_triangles(size_, e.strength, e.radius, mesh_);
      return geometry(SDL_BLENDMODE_BLEND);
    case core::EffectKind::Crt:
      scanline_triangles(size_, e.strength * 0.6f, mesh_);
      vignette_triangles(size_, e.strength * 0.8f, 0.45f, mesh_);
      return geometry(SDL_BLENDMODE_BLEND);
  }
}

// Glow: the frame at quarter size, squared (bright parts dominate), added
// back blurred at four offsets of `radius` game pixels.
void Compositor::bloom(const core::Effect& e) {
  SDL_Texture* small = canvas_ ? target(kSmall, whole(size_.x / 4), whole(size_.y / 4)) : nullptr;
  if (!small) return unavailable("bloom (no scene texture)");
  blit(canvas_, small, SDL_BLENDMODE_NONE, SDL_SCALEMODE_LINEAR);
  blit(canvas_, small, SDL_BLENDMODE_MOD, SDL_SCALEMODE_LINEAR);
  SDL_SetTextureBlendMode(small, SDL_BLENDMODE_ADD);
  SDL_SetTextureScaleMode(small, SDL_SCALEMODE_LINEAR);
  SDL_SetTextureAlphaModFloat(small, std::min(1.0f, e.strength * 0.35f));
  use(canvas_);
  const float r = e.radius;
  for (const core::Vec2 d : {core::Vec2{r, 0}, core::Vec2{-r, 0}, core::Vec2{0, r}, core::Vec2{0, -r}}) {
    const SDL_FRect dst{d.x, d.y, size_.x, size_.y};
    SDL_RenderTexture(sdl_, small, nullptr, &dst);
  }
  SDL_SetTextureAlphaModFloat(small, 1.0f);
}

void Compositor::pixelate(const core::Effect& e) {
  SDL_Texture* small = canvas_ ? target(kSmall, whole(size_.x / e.size), whole(size_.y / e.size)) : nullptr;
  if (!small) return unavailable("pixelate (no scene texture)");
  blit(canvas_, small, SDL_BLENDMODE_NONE, SDL_SCALEMODE_NEAREST);
  blit(small, canvas_, SDL_BLENDMODE_NONE, SDL_SCALEMODE_NEAREST);
}

// Color grade: the canvas multiplied by `color`, faded toward white by 1 - strength.
void Compositor::tint(const core::Effect& e) {
  const core::Color c = e.color;
  SDL_SetRenderDrawBlendMode(sdl_, SDL_BLENDMODE_MOD);
  SDL_SetRenderDrawColor(sdl_, toward_white(c.r, e.strength), toward_white(c.g, e.strength), toward_white(c.b, e.strength), 255);
  const SDL_FRect all{0, 0, size_.x, size_.y};
  SDL_RenderFillRect(sdl_, &all);
  SDL_SetRenderDrawBlendMode(sdl_, SDL_BLENDMODE_BLEND);
}

}  // namespace alpha::render
