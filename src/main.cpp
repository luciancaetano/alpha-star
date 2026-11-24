#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "core/version.hpp"

// Harness entry point: opens a window and clears it until the user closes it.
namespace {

int fail(const char* what) {
  SDL_Log("%s failed: %s", what, SDL_GetError());
  SDL_Quit();
  return 1;
}

bool quit_requested() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) return true;
  }
  return false;
}

void run_loop(SDL_Renderer* renderer) {
  while (!quit_requested()) {
    SDL_SetRenderDrawColor(renderer, 24, 24, 32, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
  }
}

}  // namespace

int main(int, char**) {
  if (!SDL_Init(SDL_INIT_VIDEO)) return fail("SDL_Init");
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  if (!SDL_CreateWindowAndRenderer("alpha-star", 960, 540, 0, &window, &renderer)) {
    return fail("SDL_CreateWindowAndRenderer");
  }
  SDL_SetRenderVSync(renderer, 1);
  SDL_Log("alpha-star %s", alpha::core::version().data());
  run_loop(renderer);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
