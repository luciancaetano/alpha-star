#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <filesystem>
#include <memory>
#include <vector>

#include "render/renderer.hpp"

using namespace alpha::core;
using alpha::render::Frame;
using alpha::render::Renderer;
using alpha::render::RenderStats;

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
  f.shapes.push_back({{Rect{{20, 20}}, kWhite, {}, {}}, {16, 16}, 0});
  f.shapes.push_back({{Circle{6}, kGreen, {}, {}}, {16, 16}, 0});
  c.renderer.draw(f);
  CHECK(c.pixel(60, 30) == kBg);
  CHECK(c.pixel(16, 16) == kGreen);  // circle over rect
  CHECK(c.pixel(8, 8) == kWhite);    // rect corner outside circle
}

TEST_CASE("images draw stretched to their rect, between shapes in z order") {
  Canvas c;
  auto img = std::make_shared<Image>(Image{1, 1, {200, 0, 200, 255}, false});
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

namespace {

// 2x1 image: left pixel red, right pixel blue.
ImagePtr two_pixels() {
  return std::make_shared<Image>(Image{2, 1, {255, 0, 0, 255, 0, 0, 255, 255}, false});
}

DrawItem sprite(const ImagePtr& img, Vec2 pos, Vec2 size, Region src = {}) {
  DrawItem item{{Rect{size}, {}, {}, img}, pos, 0};
  item.shape.src = src;
  return item;
}

const Color kRed{255, 0, 0, 255};
const Color kBlue{0, 0, 255, 255};

}  // namespace

TEST_CASE("sprites draw their source rect, mirrored by flip") {
  Canvas c;
  Frame f{kBg, {}, {}};
  f.shapes.push_back(sprite(two_pixels(), {8, 8}, {16, 16}, {1, 0, 1, 1}));
  f.shapes.push_back(sprite(two_pixels(), {40, 16}, {16, 16}));
  f.shapes.back().shape.flip_x = true;
  c.renderer.draw(f);
  CHECK(c.pixel(8, 8) == kBlue);    // only the right pixel, stretched
  CHECK(c.pixel(36, 16) == kBlue);  // mirrored: blue on the left half
  CHECK(c.pixel(44, 16) == kRed);
}

TEST_CASE("the camera view moves and zooms the world; texts stay on screen") {
  Canvas c;
  Frame f{kBg, {}, {{"#", {4, 4}, 8, kWhite, Align::Left}}};
  f.shapes.push_back({{Rect{{4, 4}}, kGreen, {}, {}}, {1000, 500}, 0});
  f.view = View{{1000, 500}, 2.0f, 0.0f, {64, 32}};  // centered on the rect, 2x
  c.renderer.draw(f);
  CHECK(c.pixel(32, 16) == kGreen);
  CHECK(c.pixel(35, 16) == kGreen);  // 4 px wide at 2x = 8 px on screen
  CHECK(c.pixel(38, 16) == kBg);
  int lit = 0;
  for (int y = 0; y < 12; ++y) {
    for (int x = 0; x < 12; ++x) lit += c.pixel(x, y) == kWhite;
  }
  CHECK(lit > 4);  // the text is not moved by the camera
}

TEST_CASE("items outside the view are culled; same-texture neighbours share one draw call") {
  Canvas c;
  const ImagePtr img = two_pixels();
  Frame f{kBg, {}, {}};
  for (int i = 0; i < 10; ++i) f.shapes.push_back(sprite(img, {static_cast<float>(i) * 4, 8}, {4, 4}));
  for (int i = 0; i < 50; ++i) f.shapes.push_back(sprite(img, {500.0f + static_cast<float>(i), 8}, {4, 4}));
  f.shapes.push_back({{Circle{3}, kGreen, {}, {}}, {50, 20}, 0});
  c.renderer.draw(f);
  CHECK(c.renderer.stats().drawn == 11);
  CHECK(c.renderer.stats().culled == 50);
  CHECK(c.renderer.stats().draw_calls == 2);  // sprites, then the plain circle
}

TEST_CASE("tile layers draw every visible cell from the tileset in one call") {
  Canvas c;
  auto grid = std::make_shared<TileGrid>();
  grid->tile = {8, 8};
  grid->tiles = {{{0, 0}, {0, 0, 1, 1}}, {{8, 0}, {1, 0, 1, 1}}, {{800, 0}, {0, 0, 1, 1}}};
  DrawItem layer{{Rect{{808, 8}}, {}, {}, two_pixels()}, {404, 4}, 0};
  layer.shape.tiles = grid;
  c.renderer.draw({kBg, {layer}, {}});
  CHECK(c.pixel(4, 4) == kRed);
  CHECK(c.pixel(12, 4) == kBlue);
  CHECK(c.pixel(20, 4) == kBg);
  CHECK(c.renderer.stats().draw_calls == 1);
}

// Measurement, not a check: run with alpha_tests -tc="*culling bench*" --no-skip.
TEST_CASE("culling bench: 20000 sprites, 1/20 of them on screen" * doctest::skip()) {
  SDL_Surface* surface = SDL_CreateSurface(320, 180, SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* sdl = SDL_CreateSoftwareRenderer(surface);
  {
    Renderer renderer{sdl, 320, 180};
    Frame f{kBg, {}, {}};
    const ImagePtr img = two_pixels();
    for (int i = 0; i < 20000; ++i) {
      f.shapes.push_back(sprite(img, {static_cast<float>(i % 400) * 16, static_cast<float>(i / 400) * 18}, {12, 12}));
    }
    const auto time = [&](View v) {
      f.view = v;
      const Uint64 start = SDL_GetTicksNS();
      for (int k = 0; k < 20; ++k) renderer.draw(f);
      SDL_FlushRenderer(sdl);
      return static_cast<double>(SDL_GetTicksNS() - start) / 20e6;
    };
    const double culled = time(View{{160, 450}, 1.0f, 0.0f, {320, 180}});
    const RenderStats on_screen = renderer.stats();
    const double all = time(View{{3200, 450}, 0.05f, 0.0f, {320, 180}});
    MESSAGE("on screen: " << on_screen.drawn << " drawn, " << on_screen.culled << " culled -> " << culled
                          << " ms/frame; everything visible (" << renderer.stats().drawn << " drawn) -> " << all
                          << " ms/frame");
  }
  SDL_DestroyRenderer(sdl);
  SDL_DestroySurface(surface);
}

TEST_CASE("pixelMode: a scaled 2x2 checker stays crisp with nearest and blends with linear") {
  const std::vector<std::uint8_t> checker{0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 255};
  auto crisp = std::make_shared<Image>(Image{2, 2, checker, false});
  auto smooth = std::make_shared<Image>(Image{2, 2, checker, true});
  Canvas c;
  Frame f{kBg, {}, {}};
  f.shapes.push_back({{Rect{{32, 32}}, {}, {}, crisp}, {16, 16}, 0});
  f.shapes.push_back({{Rect{{32, 32}}, {}, {}, smooth}, {48, 16}, 0});
  c.renderer.draw(f);
  // Just left of the middle of the top-left cell's edge: nearest = pure black, linear = gray.
  const Color nearest = c.pixel(14, 8);
  const Color linear = c.pixel(32 + 14, 8);
  CHECK(nearest.r == 0);
  CHECK(linear.r > 40);
  CHECK(linear.r < 215);
}
