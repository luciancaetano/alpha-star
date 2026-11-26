#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <optional>
#include <vector>

#include "core/lighting.hpp"
#include "core/postfx.hpp"
#include "core/vec2.hpp"
#include "render/shader_states.hpp"
#include "render/tessellate.hpp"

namespace alpha::render {

// Lighting and post effects on SDL_Renderer alone: offscreen render targets,
// blend modes (add, modulate) and vertex colors, so they work on every SDL
// renderer (D3D, Vulkan, Metal, OpenGL, GLES, software) with no shaders.
// Custom shader effects (assets.shaders) run only on SDL's GPU renderer
// (ShaderStates); elsewhere they are skipped with a warning.
// Per frame: begin() picks the canvas the world is drawn into (the window, or
// a scene texture when an effect reads the frame), light() multiplies the
// canvas by a lightmap, finish() runs the effect chain and shows the canvas.
// Without render targets (texture creation fails) lighting, bloom and
// pixelate are skipped, with one logged notice.
class Compositor {
 public:
  explicit Compositor(SDL_Renderer* sdl) : sdl_(sdl), shaders_(sdl) {}
  ~Compositor();
  Compositor(const Compositor&) = delete;
  Compositor& operator=(const Compositor&) = delete;

  void begin(core::Vec2 size, bool capture);
  void light(const std::optional<core::Lighting>& lighting, const std::vector<core::LightDisc>& lights);
  void finish(const std::vector<core::Effect>& chain);

 private:
  enum Slot { kScene, kLights, kSmall, kPost, kSlots };
  SDL_Texture* target(Slot slot, int w, int h);
  void use(SDL_Texture* target);
  void blit(SDL_Texture* from, SDL_Texture* to, SDL_BlendMode mode, SDL_ScaleMode scale);
  void geometry(SDL_BlendMode mode);
  void apply(const core::Effect& e);
  void bloom(const core::Effect& e);
  void pixelate(const core::Effect& e);
  void tint(const core::Effect& e);
  void shader(const core::Effect& e);
  void unavailable(const char* what);

  SDL_Renderer* sdl_;
  core::Vec2 size_;
  SDL_Texture* canvas_ = nullptr;  // null: the window
  std::array<SDL_Texture*, kSlots> targets_{};
  std::vector<Vertex> mesh_;
  std::vector<SDL_Vertex> vertices_;
  bool warned_ = false;
  ShaderStates shaders_;
};

}  // namespace alpha::render
