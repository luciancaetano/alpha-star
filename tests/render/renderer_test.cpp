#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <filesystem>
#include <memory>

#include "render/renderer.hpp"

using namespace alpha::core;
using alpha::render::Frame;
using alpha::render::Renderer;

namespace {

// Software renderer on a plain surface: real SDL drawing, no window needed.
struct Canvas {
  SDL_Surface* surface = SDL_CreateSurface(64, 32, SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* sdl = SDL_CreateSoftwareRenderer(surface);
  Renderer renderer{sdl, 64, 32};
  ~Canvas() {
    SDL_DestroyRenderer(sdl);
    SDL_DestroySurface(surface);
  }
  Color pixel(int x, int y) {
    SDL_FlushRenderer(sdl);
    Color c;
    SDL_ReadSurfacePixel(surface, x, y, &c.r, &c.g, &c.b, &c.a);
    return c;
  }
};

const Color kBg{10, 20, 30, 255};
const Color kGreen{0, 200, 0, 255};
const Color kWhite{255, 255, 255, 255};

}  // namespace

TEST_CASE("renderer clears to background and fills shapes, later z on top") {
  Canvas c;
  REQUIRE(c.sdl);
  Frame f{kBg, {}, {}};
  f.shapes.push_back({{Rect{{20, 20}}, kWhite, {}}, {16, 16}, 0});
  f.shapes.push_back({{Circle{6}, kGreen, {}}, {16, 16}, 0});
  c.renderer.draw(f);
  CHECK(c.pixel(60, 30) == kBg);
  CHECK(c.pixel(16, 16) == kGreen);  // circle over rect
  CHECK(c.pixel(8, 8) == kWhite);    // rect corner outside circle
}

TEST_CASE("images draw stretched to their rect, between shapes in z order") {
  Canvas c;
  auto img = std::make_shared<Image>(Image{1, 1, {200, 0, 200, 255}});
  Frame f{kBg, {}, {}};
  f.shapes.push_back({{Rect{{30, 30}}, kWhite, {}, {}}, {16, 16}, 0});
  f.shapes.push_back({{Rect{{20, 10}}, {}, {}, img}, {16, 16}, 0});
  f.shapes.push_back({{Circle{2}, kGreen, {}, {}}, {16, 16}, 0});
  c.renderer.draw(f);
  c.renderer.draw(f);  // second frame reuses the cached texture
  CHECK(c.pixel(16, 16) == kGreen);
  CHECK(c.pixel(24, 16) == Color{200, 0, 200, 255});
  CHECK(c.pixel(16, 26) == kWhite);  // below the 10px-tall image
}

TEST_CASE("renderer draws text in the text color") {
  Canvas c;
  c.renderer.draw({kBg, {}, {{"#", {32, 16}, 16, kWhite, Align::Center}}});
  int lit = 0;
  for (int y = 8; y < 24; ++y) {
    for (int x = 24; x < 40; ++x) lit += c.pixel(x, y) == kWhite;
  }
  CHECK(lit > 20);
  CHECK(c.pixel(2, 2) == kBg);
}

TEST_CASE("save_bmp writes the drawn frame") {
  Canvas c;
  c.renderer.draw({kGreen, {}, {}});
  const auto path = std::filesystem::temp_directory_path() / "alpha_renderer_test.bmp";
  REQUIRE(c.renderer.save_bmp(path.string()));
  SDL_Surface* loaded = SDL_LoadBMP(path.string().c_str());
  REQUIRE(loaded);
  Color px;
  SDL_ReadSurfacePixel(loaded, 5, 5, &px.r, &px.g, &px.b, &px.a);
  CHECK(px == kGreen);
  SDL_DestroySurface(loaded);
  std::filesystem::remove(path);
  CHECK_FALSE(c.renderer.save_bmp("Z:/no/such/dir/x.bmp"));
}
