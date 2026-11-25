#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <memory>

#include "core/glyph_layout.hpp"
#include "core/particles.hpp"
#include "platform/glyphs.hpp"
#include "render/renderer.hpp"

using namespace alpha::core;
using alpha::render::Frame;
using alpha::render::Renderer;

namespace {

// Software renderer on a plain surface (64 x 32 game pixels).
struct Canvas {
  explicit Canvas(Scaling scaling = Scaling::Letterbox, int w = 64, int h = 32)
      : surface(SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32)),
        sdl(SDL_CreateSoftwareRenderer(surface)),
        renderer(sdl, 64, 32, scaling) {}
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
  SDL_Surface* surface;
  SDL_Renderer* sdl;
  Renderer renderer;
};

const Color kBg{100, 100, 100, 255};
const Color kWhite{255, 255, 255, 255};

DrawItem box(Vec2 pos, Color fill) { return {{Rect{{10, 10}}, fill}, pos, 0}; }

bool near(Color a, Color b) {
  const auto d = [](int x, int y) { return x > y ? x - y : y - x; };
  return d(a.r, b.r) <= 2 && d(a.g, b.g) <= 2 && d(a.b, b.b) <= 2;
}

}  // namespace

TEST_CASE("tint multiplies, alpha fades, add brightens, multiply darkens") {
  Canvas c;
  Frame f{kBg, {}, {}};
  f.shapes.push_back(box({8, 8}, kWhite));
  f.shapes.back().shape.tint = {255, 0, 0, 255};
  f.shapes.push_back(box({24, 8}, {200, 200, 200, 255}));
  f.shapes.back().shape.alpha = 0.5f;
  f.shapes.push_back(box({40, 8}, {50, 50, 50, 255}));
  f.shapes.back().shape.blend = Blend::Add;
  f.shapes.push_back(box({56, 8}, {128, 128, 128, 255}));
  f.shapes.back().shape.blend = Blend::Multiply;
  c.renderer.draw(f);
  CHECK(near(c.pixel(8, 8), {255, 0, 0, 255}));
  CHECK(near(c.pixel(24, 8), {150, 150, 150, 255}));
  CHECK(near(c.pixel(40, 8), {150, 150, 150, 255}));
  CHECK(near(c.pixel(56, 8), {50, 50, 50, 255}));
  CHECK(c.renderer.stats().draw_calls == 3);  // alpha, alpha, add, multiply: the first two share
}

TEST_CASE("particles draw as one batch in their colors, culled per particle") {
  Canvas c;
  DrawItem item{EmitterSpec{}.look, {0, 0}, 0};
  item.particles = {{{10, 10}, 6, {255, 0, 0, 255}}, {{30, 10}, 6, {0, 0, 255, 255}}, {{900, 10}, 6, kWhite}};
  c.renderer.draw({kBg, {item}, {}});
  CHECK(c.pixel(10, 10) == Color{255, 0, 0, 255});
  CHECK(c.pixel(30, 10) == Color{0, 0, 255, 255});
  CHECK(c.pixel(20, 10) == kBg);
  CHECK(c.renderer.stats().draw_calls == 1);
}

TEST_CASE("a layer's parallax moves it less than the camera") {
  Canvas c;
  Frame f{kBg, {}, {}};
  f.shapes.push_back(box({32, 24}, kWhite));          // world layer
  f.shapes.push_back(box({32, 8}, {0, 255, 0, 255}));  // fixed layer
  f.shapes.back().parallax = 0.0f;
  f.view = View{{52, 16}, 1.0f, 0.0f, {64, 32}};  // camera 20 px right of home
  c.renderer.draw(f);
  CHECK(c.pixel(12, 24) == kWhite);              // moved 20 px left
  CHECK(c.pixel(32, 8) == Color{0, 255, 0, 255});  // stayed
  CHECK(c.renderer.stats().draw_calls == 2);     // parallax breaks the batch
}

TEST_CASE("world text: font atlas glyphs in the text color, and the debug-font fallback through the camera") {
  Canvas c;
  const FontAtlasPtr font = alpha::platform::rasterize_font(ALPHA_TEST_FONT, 8);
  TextQuads q = layout_text(*font, "##", Align::Center);
  DrawItem text{{Rect{q.box}, {255, 0, 0, 255}, {}, font->image}, {116, 16}, 0};
  text.shape.tiles = std::make_shared<TileGrid>(q.grid);
  text.shape.text = TextLook{"##", "pixel", 8, Align::Center};
  DrawItem debug{{Rect{debug_text_box("#", 8, Align::Center)}, {0, 0, 255, 255}}, {148, 16}, 0};
  debug.shape.text = TextLook{"#", "", 8, Align::Center};
  Frame f{kBg, {text, debug}, {}};
  f.view = View{{132, 16}, 1.0f, 0.0f, {64, 32}};  // camera 100 px right: text at x 16, debug text at x 48
  c.renderer.draw(f);
  int red = 0;
  int blue = 0;
  for (int y = 10; y < 22; ++y) {
    for (int x = 8; x < 60; ++x) {
      red += c.pixel(x, y) == Color{255, 0, 0, 255};
      blue += c.pixel(x, y) == Color{0, 0, 255, 255};
    }
  }
  CHECK(red > 20);
  CHECK(blue > 10);
  CHECK_THROWS_WITH(alpha::platform::rasterize_font("no/such.ttf", 8), doctest::Contains("cannot read the file"));
}

TEST_CASE("scaling modes: integer and stretch keep the logical size; expand grows it to the output's aspect") {
  Canvas integer(Scaling::Integer, 150, 80);
  CHECK(integer.renderer.logical_size() == Vec2{64, 32});
  SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
  int w = 0;
  int h = 0;
  SDL_GetRenderLogicalPresentation(integer.sdl, &w, &h, &mode);
  CHECK(mode == SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
  Canvas stretch(Scaling::Stretch, 100, 100);
  SDL_GetRenderLogicalPresentation(stretch.sdl, &w, &h, &mode);
  CHECK(mode == SDL_LOGICAL_PRESENTATION_STRETCH);
  Canvas expand(Scaling::Expand, 128, 128);
  expand.renderer.draw({kBg, {}, {}});
  CHECK(expand.renderer.logical_size() == Vec2{64, 64});
  CHECK(expand.pixel(32, 60) == kBg);  // the grown area is drawn, no bars
}

// Measurement, not a check: alpha_tests -tc="*particle render bench*" --no-skip.
TEST_CASE("particle render bench: 20000 particles, software renderer" * doctest::skip()) {
  Canvas c(Scaling::Letterbox, 64, 32);
  DrawItem item{EmitterSpec{}.look, {0, 0}, 0};
  for (int i = 0; i < kMaxParticles; ++i) {
    item.particles.push_back({{static_cast<float>(i % 64), static_cast<float>(i % 32)}, 2, kWhite});
  }
  item.shape.geometry = Rect{{1, 1}};
  const Uint64 t0 = SDL_GetTicksNS();
  for (int i = 0; i < 60; ++i) c.renderer.draw({kBg, {item}, {}});
  SDL_FlushRenderer(c.sdl);
  MESSAGE("20000 square particles: ", static_cast<double>(SDL_GetTicksNS() - t0) / 60e6, " ms/frame, ",
          c.renderer.stats().draw_calls, " draw call(s)");
}
