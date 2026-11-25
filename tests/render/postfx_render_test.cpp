#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include "core/lighting.hpp"
#include "core/postfx.hpp"
#include "render/renderer.hpp"

using namespace alpha::core;
using alpha::render::Frame;
using alpha::render::Renderer;

// Lighting and post effects on the software renderer (what headless tests,
// CI and `--hidden` screenshots use): every effect changes pixels there too.
namespace {

struct Canvas {
  Canvas() : surface(SDL_CreateSurface(64, 32, SDL_PIXELFORMAT_RGBA32)), sdl(SDL_CreateSoftwareRenderer(surface)), renderer(sdl, 64, 32) {}
  ~Canvas() {
    SDL_DestroyRenderer(sdl);
    SDL_DestroySurface(surface);
  }
  Canvas(const Canvas&) = delete;
  Canvas& operator=(const Canvas&) = delete;
  Color pixel(int x, int y) {
    SDL_FlushRenderer(sdl);
    Color c;
    SDL_ReadSurfacePixel(surface, x, y, &c.r, &c.g, &c.b, &c.a);
    return c;
  }
  Color draw(const Frame& f, int x, int y) {
    renderer.draw(f);
    return pixel(x, y);
  }
  SDL_Surface* surface;
  SDL_Renderer* sdl;
  Renderer renderer;
};

const Color kGray{128, 128, 128, 255};

// A white 4 x 4 square at (10, 10) on gray.
Frame scene() {
  Frame f{kGray, {}, {}};
  f.shapes.push_back({{Rect{{4, 4}}, {255, 255, 255, 255}}, {10, 10}, 0});
  return f;
}

}  // namespace

TEST_CASE("no effects and no lighting leave the frame as drawn") {
  Canvas c;
  CHECK(c.draw(scene(), 40, 20) == kGray);
  CHECK(c.pixel(10, 10) == Color{255, 255, 255, 255});
}

TEST_CASE("lighting darkens to ambient and a light brightens its center") {
  Canvas c;
  Frame f = scene();
  f.lighting = Lighting{{64, 64, 64, 255}};
  f.lights.push_back({{48, 16}, {12, {255, 255, 255, 255}, 1.0f}});
  CHECK(c.draw(f, 2, 2).r < 40);                // 128 * 64/255
  CHECK(c.pixel(48, 16).r > 120);               // ambient + full light
  CHECK(c.pixel(55, 16).r > c.pixel(60, 30).r);  // falloff
}

TEST_CASE("vignette darkens corners, not the center; crt darkens odd rows") {
  Canvas c;
  Frame f = scene();
  f.effects = {make_effect("vignette", {{"strength", 1.0f}, {"radius", 0.2f}})};
  CHECK(c.draw(f, 0, 0).r < 40);
  CHECK(c.pixel(32, 16).r > 120);
  f.effects = {make_effect("crt", {{"strength", 1.0f}})};
  c.draw(f, 0, 0);
  CHECK(c.pixel(32, 15).r < c.pixel(32, 16).r);
}

TEST_CASE("tint multiplies by its color") {
  Canvas c;
  Frame f = scene();
  f.effects = {make_effect("tint", {}, Color{255, 0, 0, 255})};
  const Color p = c.draw(f, 40, 20);
  CHECK(p.r == 128);
  CHECK(p.g == 0);
}

TEST_CASE("pixelate makes blocks; bloom brightens around bright shapes") {
  Canvas c;
  Frame f = scene();
  f.effects = {make_effect("pixelate", {{"size", 16.0f}})};
  c.draw(f, 0, 0);
  for (int x = 0; x < 16; ++x) CHECK(c.pixel(x, 3) == c.pixel(0, 0));  // one 16 px block
  Frame dark{{0, 0, 0, 255}, {}, {}};
  dark.shapes.push_back({{Rect{{8, 8}}, {255, 255, 255, 255}}, {32, 16}, 0});
  CHECK(c.draw(dark, 38, 16).r == 0);
  dark.effects = {make_effect("bloom", {{"strength", 2.0f}})};
  CHECK(c.draw(dark, 38, 16).r > 20);
  CHECK(c.pixel(32, 16).r == 255);  // the shape itself stays
}
