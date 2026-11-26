#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <filesystem>
#include <memory>
#include <string>

#include "core/postfx.hpp"
#include "platform/shader_files.hpp"
#include "platform/window.hpp"
#include "render/renderer.hpp"

using namespace alpha::core;
using alpha::render::Frame;
using alpha::render::Renderer;

// Custom shader effects: skipped (not crashed) without the GPU renderer; run on it.
namespace {

const Color kGray{100, 100, 100, 255};

// The fixture's blobs, read as a pack would be (no .frag next to them is needed).
Effect invert(float amount) {
  Shader s = make_shader("invert", "invert.frag", {{"amount", 1.0f}});
  s.blobs = alpha::platform::load_shader_blobs(std::filesystem::path(ALPHA_TEST_FIXTURES) / "shaders", s.file,
                                               "assets.shaders.invert");
  return make_shader_effect(std::make_shared<const Shader>(std::move(s)), {{"amount", amount}});
}

Color read(SDL_Renderer* sdl, int x, int y) {
  const SDL_Rect r{x, y, 1, 1};
  SDL_Surface* s = SDL_RenderReadPixels(sdl, &r);
  Color c;
  if (s) SDL_ReadSurfacePixel(s, 0, 0, &c.r, &c.g, &c.b, &c.a);
  SDL_DestroySurface(s);
  return c;
}

}  // namespace

TEST_CASE("a shader effect on the software renderer is skipped; the frame stays as drawn") {
  SDL_Surface* surface = SDL_CreateSurface(64, 32, SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* sdl = SDL_CreateSoftwareRenderer(surface);
  {
    Renderer renderer(sdl, 64, 32);
    Frame f{kGray, {}, {}};
    f.effects = {invert(1.0f)};
    renderer.draw(f);
    renderer.draw(f);  // warned once, still no effect
    SDL_FlushRenderer(sdl);
    Color c;
    SDL_ReadSurfacePixel(surface, 5, 5, &c.r, &c.g, &c.b, &c.a);
    CHECK(c == kGray);
  }
  SDL_DestroyRenderer(sdl);
  SDL_DestroySurface(surface);
}

TEST_CASE("on the Vulkan GPU renderer a shader effect runs over the frame with its uniforms") {
  SDL_Window* window = SDL_CreateWindow("shader test", 64, 32, SDL_WINDOW_HIDDEN);
  SDL_Renderer* sdl = window ? alpha::platform::vulkan_renderer(window) : nullptr;
  if (!sdl) {
    MESSAGE("no Vulkan GPU renderer here (" << std::string(SDL_GetError()) << "): skipped");
    SDL_DestroyWindow(window);
    return;
  }
  {
    Renderer renderer(sdl, 64, 32);
    Frame f{kGray, {}, {}};
    f.effects = {invert(1.0f)};
    renderer.draw(f);
    const Color inverted = read(sdl, 5, 5);
    CHECK(inverted.r >= 153);
    CHECK(inverted.r <= 157);
    f.effects = {invert(0.0f)};
    renderer.draw(f);
    CHECK(read(sdl, 5, 5).r <= 102);
  }
  SDL_DestroyRenderer(sdl);
  SDL_DestroyWindow(window);
}
