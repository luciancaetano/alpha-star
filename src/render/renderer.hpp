#pragma once

#include <SDL3/SDL.h>

#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/entity_spec.hpp"
#include "render/tessellate.hpp"

namespace alpha::render {

// Everything drawn in one frame, in game coordinates.
struct Frame {
  core::Color background{0, 0, 0, 255};
  std::vector<core::DrawItem> shapes;  // already ordered (World::draw_items)
  std::vector<core::TextItem> texts;   // drawn over shapes, in order
};

// Draws frames on an SDL renderer it does not own. The game's logical size
// is letterboxed into the output, so scripts only ever see game pixels.
class Renderer {
 public:
  Renderer(SDL_Renderer* sdl, int width, int height);
  ~Renderer();
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  void draw(const Frame& frame);
  void present();
  // Saves what draw() produced, before present(). Returns false on failure.
  bool save_bmp(const std::string& path) const;

 private:
  void draw_shapes(const std::vector<core::DrawItem>& shapes);
  void draw_geometry(std::span<const core::DrawItem> items);
  void draw_image(const core::DrawItem& item);
  SDL_Texture* texture(const core::ImagePtr& image);
  void draw_text(const core::TextItem& text);

  SDL_Renderer* sdl_;
  std::vector<SDL_Vertex> vertices_;
  // Images live as long as the game; textures are created on first draw.
  std::unordered_map<const core::Image*, SDL_Texture*> textures_;
};

}  // namespace alpha::render
