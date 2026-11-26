#pragma once

#include <SDL3/SDL.h>

#include <map>
#include <memory>
#include <set>
#include <string>

#include "core/postfx.hpp"

namespace alpha::render {

// GPU objects for custom shader effects (assets.shaders), made on first use
// and freed once the game no longer holds the shader (hot reload). Needs SDL's
// GPU renderer (SDL_GPURenderState); on any other renderer, or when no blob
// fits the device, the effect is skipped with one logged warning per shader.
class ShaderStates {
 public:
  explicit ShaderStates(SDL_Renderer* sdl) : sdl_(sdl) {}
  ~ShaderStates();
  ShaderStates(const ShaderStates&) = delete;
  ShaderStates& operator=(const ShaderStates&) = delete;

  // The render state for a shader effect with its uniforms set, or null.
  SDL_GPURenderState* state(const core::Effect& effect);
  // Frees the objects of shaders nobody holds any more (once per frame).
  void prune();

 private:
  struct Entry {
    std::weak_ptr<const core::Shader> owner;
    SDL_GPUShader* shader = nullptr;
    SDL_GPURenderState* state = nullptr;
  };
  Entry make(const core::ShaderPtr& shader);
  Entry create(SDL_GPUDevice* device, Entry e, core::ShaderFormat format);
  void warn(const core::Shader& shader, const std::string& why);
  void release(Entry& e);

  SDL_Renderer* sdl_;
  std::map<const core::Shader*, Entry> entries_;
  std::set<std::string> warned_;
};

}  // namespace alpha::render
