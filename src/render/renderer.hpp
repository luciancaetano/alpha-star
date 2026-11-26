#pragma once

#include <SDL3/SDL.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/camera.hpp"
#include "core/entity_spec.hpp"
#include "core/window.hpp"
#include "core/lighting.hpp"
#include "core/pixel_grid.hpp"
#include "core/postfx.hpp"
#include "render/compositor.hpp"
#include "render/font_cache.hpp"
#include "render/tessellate.hpp"

namespace alpha::render {

// Everything drawn in one frame. Shapes are in world coordinates, seen
// through `view` (none = the identity camera); texts are screen overlays in
// game pixels and never move with the camera.
struct Frame {
  Frame(core::Color bg = {0, 0, 0, 255}, std::vector<core::DrawItem> s = {}, std::vector<core::TextItem> t = {});

  core::Color background;
  std::vector<core::DrawItem> shapes;  // already ordered (World::draw_items)
  std::vector<core::TextItem> texts;   // drawn over shapes, in order
  std::optional<core::View> view;
  // Render-only looks: lights (with `lighting` set) multiply the world by a
  // lightmap; `effects` then run over the lit world, in order. Texts stay unaffected.
  std::optional<core::Lighting> lighting;
  std::vector<core::LightItem> lights;
  std::vector<core::Effect> effects;
};

// What the last draw() did: shapes drawn / skipped as off-screen, and
// SDL_RenderGeometry calls (one per run of same-texture items).
struct RenderStats {
  int drawn = 0;
  int culled = 0;
  int draw_calls = 0;
  int vertices = 0;  // world geometry sent to SDL (game UI and dev tools not counted)
};

// Draws frames on an SDL renderer it does not own. The game's logical size
// is presented into the output by `stretch` (core::present), so scripts only
// ever see game pixels; the revealed axes follow the window.
// Batches break on texture, blend mode or layer parallax.
class Renderer {
 public:
  Renderer(SDL_Renderer* sdl, int width, int height, core::Stretch stretch = {});
  ~Renderer();
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  void draw(const Frame& frame);
  const RenderStats& stats() const { return stats_; }
  void present();
  SDL_Renderer* sdl() const { return sdl_; }
  // Game pixels the screen shows now (the game size grown along the revealed axes).
  core::Vec2 logical_size() const { return size_; }
  // Output pixels at the last draw (the window, high-DPI included).
  core::Vec2 output_size() const { return output_; }
  // Saves what draw() produced, before present(). Returns false on failure.
  bool save_bmp(const std::string& path) const;

 private:
  // What a batch shares: texture (or none), blend mode, layer parallax.
  struct BatchKey {
    SDL_Texture* texture = nullptr;
    core::Blend blend = core::Blend::Alpha;
    float parallax = 1.0f;
    bool operator==(const BatchKey&) const = default;
  };

  void present_logical();
  void draw_shapes(const std::vector<core::DrawItem>& shapes, const core::View& view);
  void add(const core::DrawItem& item, const core::View& view);
  void flush(const core::View& view);
  SDL_Texture* texture(const core::ImagePtr& image);
  SDL_Texture* white();
  void draw_text(const core::TextItem& text);
  void add_text(const core::DrawItem& item, const core::View& view);
  void batch(const BatchKey& key, const core::View& view);
  void forget(const core::Image* image);

  SDL_Renderer* sdl_;
  core::Vec2 game_size_;
  core::Vec2 size_;
  core::Vec2 output_;
  core::Stretch stretch_;
  core::PixelGrid grid_;       // game pixels -> output pixels, this frame
  float window_scale_ = 1.0f;  // output pixels per game pixel
  RenderStats stats_;
  // The batch being built: world-space triangles sharing one key.
  std::vector<Vertex> batch_;
  BatchKey batch_key_;
  std::vector<SDL_Vertex> vertices_;
  // Images live as long as the game; textures are created on first draw.
  std::unordered_map<const core::Image*, SDL_Texture*> textures_;
  // World-text atlases at output sizes; an evicted one frees its texture.
  FontCache fonts_;
  std::vector<SDL_Texture*> doomed_;  // evicted this frame
  // 1x1 white: carries add/multiply for untextured batches (SDL 3.4's software
  // renderer draws untextured rects with alpha blend whatever the draw blend mode;
  // drawn with wrap addressing so it takes the triangle path).
  SDL_Texture* white_ = nullptr;
  Compositor compositor_;
};

}  // namespace alpha::render
