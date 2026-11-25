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

namespace {

// 2x1 image: left pixel red, right pixel blue.
ImagePtr two_pixels() {
  return std::make_shared<Image>(Image{2, 1, {255, 0, 0, 255, 0, 0, 255, 255}});
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

// A 2x2 black/white checker drawn 16x (32 px) at `x` with `filter`.
void checker(Frame& f, Filter filter, float x) {
  const std::vector<std::uint8_t> px{0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 255};
  f.shapes.push_back({{Rect{{32, 32}}, {}, {}, std::make_shared<Image>(Image{2, 2, px, filter})}, {x, 16}, 0});
}

TEST_CASE("image filter: nearest stays hard, linear blends whole texels, sharp blends only their borders") {
  Canvas c;
  Frame f{kBg, {}, {}};
  checker(f, Filter::nearest, 16);
  checker(f, Filter::linear, 48);
  c.renderer.draw(f);
  // 4 px inside the black cell's right edge (x = 16): linear is already gray.
  CHECK(c.pixel(12, 8).r == 0);
  CHECK(c.pixel(32 + 12, 8).r > 40);
  CHECK(c.pixel(15, 8).r == 0);

  Canvas s;
  Frame g{kBg, {}, {}};
  checker(g, Filter::sharp, 16);
  s.renderer.draw(g);
  CHECK(s.pixel(12, 8).r == 0);   // the cell's inside stays solid
  CHECK(s.pixel(15, 8).r > 40);   // its last pixel before the edge blends
  CHECK(s.pixel(15, 8).r < 215);
}

TEST_CASE("smooth shapes: edge pixels are partially covered; crisp edges are fill or background") {
  Canvas c;
  Frame f{kBg, {}, {}};
  ShapeSpec smooth{Circle{7.6f}, kGreen, {}, {}};
  smooth.smooth = true;
  f.shapes.push_back({{Circle{7.6f}, kGreen, {}, {}}, {16, 16.5f}, 0});
  f.shapes.push_back({smooth, {48, 16.5f}, 0});
  c.renderer.draw(f);
  const auto partial = [](Color p) { return p.g > kBg.g + 20 && p.g < kGreen.g - 20; };
  int crisp_partial = 0;
  int smooth_partial = 0;
  for (int x = 0; x < 32; ++x) {
    crisp_partial += partial(c.pixel(x, 16));
    smooth_partial += partial(c.pixel(32 + x, 16));
  }
  CHECK(crisp_partial == 0);
  CHECK(smooth_partial >= 2);  // both sides of the row
  CHECK(c.pixel(16, 16) == kGreen);
  CHECK(c.pixel(48, 16) == kGreen);
}
