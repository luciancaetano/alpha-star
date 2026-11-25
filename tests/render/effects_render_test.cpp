#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <memory>
#include <string>

#include "core/glyph_layout.hpp"
#include "core/particles.hpp"
#include "platform/fonts.hpp"
#include "render/font_cache.hpp"
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

namespace {

// A text item as the script layer makes it.
DrawItem text_item(const FontSourcePtr& face, const std::string& text, float size, Vec2 pos) {
  DrawItem item{{Rect{{64, 32}}, kWhite}, pos, 0};
  item.shape.text = TextLook{text, "f", face, size, Align::Center};
  return item;
}

// Pixels in the box that are neither the background nor exactly `ink`.
int blended(Canvas& c, Color ink, int w, int h) {
  int n = 0;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) n += c.pixel(x, y) != kBg && c.pixel(x, y) != ink;
  }
  return n;
}

int inked(Canvas& c, Color ink, int w, int h) {
  int n = 0;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) n += c.pixel(x, y) == ink;
  }
  return n;
}

// Rows whose white pixels form one run exactly `width` long.
int solid_rows(Canvas& c, int width) {
  int rows = 0;
  for (int y = 0; y < 48; ++y) {
    int first = -1;
    int last = -1;
    int n = 0;
    for (int x = 0; x < 96; ++x) {
      if (c.pixel(x, y) != kWhite) continue;
      first = first < 0 ? x : first;
      last = x;
      ++n;
    }
    rows += n == width && last - first + 1 == width;
  }
  return rows;
}

}  // namespace

TEST_CASE("world text at 1.5x window scale: rasterized at the output size, every edge on a whole pixel") {
  Canvas c(Scaling::Letterbox, 96, 48);  // 64 x 32 game pixels shown 1.5x
  // Press Start 2P is drawn on an 8-unit grid: at 16 game px = 24 output px its
  // edges are whole output pixels, unless something resamples it.
  const FontSourcePtr smooth = alpha::platform::load_font(ALPHA_TEST_FONT, false);
  c.renderer.draw({kBg, {text_item(smooth, "#H", 16, {32.3f, 16.2f})}, {}});
  CHECK(inked(c, kWhite, 96, 48) > 100);
  CHECK(blended(c, kWhite, 96, 48) == 0);
  Frame zoomed{kBg, {text_item(smooth, "#", 8, {32, 16})}, {}};
  zoomed.view = View{{32, 16}, 2.0f, 0.0f, {64, 32}};  // 8 px x zoom 2 x 1.5 = 24 output px
  c.renderer.draw(zoomed);
  CHECK(inked(c, kWhite, 96, 48) > 100);
  CHECK(blended(c, kWhite, 96, 48) == 0);
}

TEST_CASE("bitmap fonts draw whole texels: BMFont with kerning, and the built-in font with no file") {
  Canvas c(Scaling::Letterbox, 96, 48);
  const FontSourcePtr tiny = alpha::platform::load_font(ALPHA_TEST_FIXTURES "/fonts/tiny.fnt", true);
  CHECK(tiny->native() == 4);
  c.renderer.draw({kBg, {text_item(tiny, "AV", 4, {32, 16})}, {}});
  // 4 px x 1.5 -> 2 output px per texel: two 3x4 blocks, kerned to touch (advance 4 - 1)
  // Every inked row is one run of 12 px: no gap, so the kerning pair applied.
  // 4 texel rows -> 8 px; SDL's software renderer turns this quad into a blit
  // whose float->int height can lose the last row (GPUs sample pixel centers).
  const int rows = solid_rows(c, 12);
  CHECK(inked(c, kWhite, 96, 48) == rows * 12);
  CHECK((rows == 8 || rows == 7));
  CHECK(blended(c, kWhite, 96, 48) == 0);
  c.renderer.draw({kBg, {text_item(nullptr, "Hi", 16, {32, 16})}, {}});  // no face: built-in 8x8
  CHECK(inked(c, kWhite, 96, 48) > 40);
  CHECK(blended(c, kWhite, 96, 48) == 0);
  CHECK_THROWS_WITH(alpha::platform::load_font("no/such.ttf", true), doctest::Contains("cannot read the file"));
  CHECK_THROWS_WITH(alpha::platform::load_font(ALPHA_TEST_FIXTURES "/fonts/tiny.png", true),
                    doctest::Contains("not a font"));
}

TEST_CASE("world text atlases are cached per size and bounded; a resize makes new ones") {
  const FontSourcePtr tiny = alpha::platform::load_font(ALPHA_TEST_FIXTURES "/fonts/tiny_xml.fnt", true);
  int evicted = 0;
  alpha::render::FontCache cache(2, [&](const Image*) { ++evicted; });
  const FontSourcePtr smooth = alpha::platform::load_font(ALPHA_TEST_FONT, false);
  const auto a = cache.get(smooth, 10);
  CHECK(cache.get(smooth, 10) == a);
  cache.get(smooth, 11);
  cache.get(tiny, 4);
  CHECK(cache.size() == 2);
  CHECK(evicted == 1);  // the 10 px atlas
  CHECK(cache.get(smooth, 10) != a);
  CHECK(tiny->atlas(99)->kerning.at({U'A', U'V'}) == -1);
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
