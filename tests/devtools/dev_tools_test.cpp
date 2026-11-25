#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include "devtools/dev_tools.hpp"
#include "host_fixture.hpp"

using alpha::devtools::DevTools;
using alpha::devtools::FrameStats;
using alpha::script::Host;
using alpha::test::Sandbox;
using alpha::test::tiny;

namespace {

// ImGui on the dummy video driver, drawn by the software renderer into a surface.
struct Screen {
  bool video = SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy") && SDL_InitSubSystem(SDL_INIT_VIDEO);
  SDL_Window* window = SDL_CreateWindow("devtools", 640, 480, SDL_WINDOW_HIDDEN);
  SDL_Surface* surface = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* sdl = SDL_CreateSoftwareRenderer(surface);
  ~Screen() {
    SDL_DestroyRenderer(sdl);
    SDL_DestroySurface(surface);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
  }
  // Pixels that differ from black after drawing.
  int lit() {
    SDL_FlushRenderer(sdl);
    int count = 0;
    for (int y = 0; y < 480; y += 4) {
      for (int x = 0; x < 640; x += 4) {
        Uint8 r = 0, g = 0, b = 0, a = 0;
        SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a);
        count += (r | g | b) != 0 ? 1 : 0;
      }
    }
    return count;
  }
  void clear() {
    SDL_SetRenderDrawColor(sdl, 0, 0, 0, 255);
    SDL_RenderClear(sdl);
  }
};

SDL_Event f1() {
  SDL_Event e{};
  e.type = SDL_EVENT_KEY_DOWN;
  e.key.scancode = SDL_SCANCODE_F1;
  e.key.down = true;
  return e;
}

SDL_Event space() {
  SDL_Event e = f1();
  e.key.scancode = SDL_SCANCODE_SPACE;
  return e;
}

const char* kGame = "ctx.spawn({ tags: ['ball'], pos: [10, 10], shape: { circle: 4, color: '#fff' },"
                    " body: { type: 'dynamic', collider: { circle: 4 } } }); console.log('hello');";

}  // namespace

TEST_CASE("dev tools start closed, F1 toggles them and never reaches the game") {
  Screen screen;
  REQUIRE(screen.video);
  DevTools tools(screen.window, screen.sdl);
  CHECK_FALSE(tools.open());
  CHECK(tools.handle(space()));
  CHECK_FALSE(tools.handle(f1()));
  CHECK(tools.open());
  CHECK_FALSE(tools.handle(f1()));
  CHECK_FALSE(tools.open());
}

TEST_CASE("open dev tools draw their windows over the game; closed ones draw nothing") {
  Screen screen;
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", tiny("enter", kGame)));
  DevTools tools(screen.window, screen.sdl);
  screen.clear();
  tools.draw(host, FrameStats{16.6, 1});
  CHECK(screen.lit() == 0);
  tools.toggle();
  tools.panels().selected = host.world()->entities().front();
  for (int i = 0; i < 3; ++i) {  // ImGui lays windows out over the first frames
    screen.clear();
    tools.draw(host, FrameStats{16.6, 1});
  }
  CHECK(screen.lit() > 100);
}

TEST_CASE("physics view adds collider outlines to the frame only while enabled and open") {
  Screen screen;
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", tiny("enter", kGame)));
  DevTools tools(screen.window, screen.sdl);
  alpha::render::Frame frame = host.frame();
  const auto shapes = frame.shapes.size();
  tools.panels().physics_view = true;
  tools.decorate(frame, host.world());
  CHECK(frame.shapes.size() == shapes);
  tools.toggle();
  tools.decorate(frame, host.world());
  CHECK(frame.shapes.size() == shapes + 1);
  tools.panels().physics_view = false;
  tools.decorate(frame, host.world());
  CHECK(frame.shapes.size() == shapes + 1);
}

TEST_CASE("time controls hold the simulation while paused") {
  Screen screen;
  DevTools tools(screen.window, screen.sdl);
  CHECK(tools.steps(2) == 2);
  tools.time().toggle_pause();
  CHECK(tools.steps(2) == 0);
}
