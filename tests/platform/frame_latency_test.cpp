#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include "platform/frame_latency.hpp"

// The Direct3D 11 path needs a real GPU window, so it is verified by the startup
// log line ("frame queue: 1 frame"); here: other backends are left untouched.
TEST_CASE("limit_frame_queue leaves non-Direct3D-11 renderers alone") {
  SDL_Surface* surface = SDL_CreateSurface(8, 8, SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* software = SDL_CreateSoftwareRenderer(surface);
  REQUIRE(software);
  CHECK_FALSE(alpha::platform::limit_frame_queue(software));
  SDL_DestroyRenderer(software);
  SDL_DestroySurface(surface);
}
