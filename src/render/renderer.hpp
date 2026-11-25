#pragma once

#include <SDL3/SDL.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/camera.hpp"
#include "core/entity_spec.hpp"
#include "core/window.hpp"
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
};

// What the last draw() did: shapes drawn / skipped as off-screen, and
// SDL_RenderGeometry calls (one per run of same-texture items).
struct RenderStats {
  int drawn = 0;
  int culled = 0;
  int draw_calls = 0;
};

// Draws frames on an SDL renderer it does not own. The game's logical size
// is presented into the output by `scaling` (core::Scaling), so scripts only
// ever see game pixels; with expand the logical size follows the window.
// Batches break on texture, blend mode or layer parallax.
class Renderer {
 public:
  Renderer(SDL_Renderer* sdl, int width, int height, core::Scaling scaling = core::Scaling::Letterbox);
  ~Renderer();
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  void draw(const Frame& frame);
  const RenderStats& stats() const { return stats_; }
  void present();
  SDL_Renderer* sdl() const { return sdl_; }
  // Game pixels the screen shows now (the game size, except with expand).
  core::Vec2 logical_size() const { return size_; }
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
  float output_scale() const;
  void draw_shapes(const std::vector<core::DrawItem>& shapes, const core::View& view);
  void add(const core::DrawItem& item, const core::View& view);
  void flush(const core::View& view);
  SDL_Texture* texture(const core::ImagePtr& image);
  void draw_text(const core::TextItem& text);
  void draw_world_text(const core::DrawItem& item, const core::View& view);

  SDL_Renderer* sdl_;
  core::Vec2 game_size_;
  core::Vec2 size_;
  core::Scaling scaling_;
  float window_scale_ = 1.0f;  // output pixels per game pixel
  RenderStats stats_;
  // The batch being built: world-space triangles sharing one key.
  std::vector<Vertex> batch_;
  BatchKey batch_key_;
  std::vector<SDL_Vertex> vertices_;
  // Images live as long as the game; textures are created on first draw.
  std::unordered_map<const core::Image*, SDL_Texture*> textures_;
};

}  // namespace alpha::render
