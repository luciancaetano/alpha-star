#pragma once

// A real game (title menu -> play) on a software renderer, for the agent
// control channel tests.
#include <SDL3/SDL.h>

#include <string>

#include "host_fixture.hpp"
#include "render/renderer.hpp"
#include "script/host.hpp"

namespace alpha::test {

inline constexpr float kStepDt = 1.0f / 60.0f;

struct McpGame {
  Sandbox box;
  script::Host host{box.save()};
  SDL_Surface* surface = SDL_CreateSurface(64, 64, SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* sdl = SDL_CreateSoftwareRenderer(surface);
  render::Renderer renderer{sdl, 100, 100};
  McpGame();
  ~McpGame();
  McpGame(const McpGame&) = delete;
  McpGame& operator=(const McpGame&) = delete;
  // Every console line so far, one per line.
  std::string logs() const;
};

}  // namespace alpha::test
