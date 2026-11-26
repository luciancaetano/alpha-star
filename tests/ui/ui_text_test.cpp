#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <string>

#include "host_fixture.hpp"
#include "ui/library.hpp"
#include "ui/ui.hpp"

using alpha::test::Sandbox;
using alpha::ui::Ui;

namespace {

// A 64 x 32 UI shown 1.5x (96 x 48 output pixels) by a letterbox presentation.
struct Screen {
  Screen() : surface(SDL_CreateSurface(96, 48, SDL_PIXELFORMAT_RGBA32)), sdl(SDL_CreateSoftwareRenderer(surface)) {
    SDL_SetRenderLogicalPresentation(sdl, 64, 32, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  }
  ~Screen() {
    SDL_DestroyRenderer(sdl);
    SDL_DestroySurface(surface);
  }
  Screen(const Screen&) = delete;
  Screen& operator=(const Screen&) = delete;

  void draw(Ui& ui) {
    SDL_SetRenderDrawColor(sdl, 0, 0, 0, 255);
    SDL_RenderClear(sdl);
    ui.render(sdl);
    SDL_FlushRenderer(sdl);
  }
  // Pixels exactly white, and pixels neither black nor white (resampled edges).
  std::pair<int, int> count() const {
    int white = 0;
    int blended = 0;
    for (int y = 0; y < 48; ++y) {
      for (int x = 0; x < 96; ++x) {
        Uint8 r = 0;
        Uint8 g = 0;
        Uint8 b = 0;
        Uint8 a = 0;
        SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a);
        white += r == 255 && g == 255 && b == 255;
        blended += r != g || (r != 0 && r != 255);
      }
    }
    return {white, blended};
  }
  SDL_Surface* surface;
  SDL_Renderer* sdl;
};

std::string page(const std::string& style, const std::string& text) {
  return "<rml><head><style>body { width: 100%; height: 100%; color: #ffffff; " + style +
         " }</style></head><body><p id=\"t\">" + text + "</p></body></rml>";
}

}  // namespace

TEST_CASE("ui text at 1.5x: TrueType made at the output size, every glyph edge on a whole output pixel") {
  Sandbox box;
  Ui ui{64, 32};
  // Press Start 2P has an 8-unit pixel grid: 16 UI px x 1.5 = 24 output px puts every edge on a pixel.
  alpha::ui::load_font(ALPHA_TEST_FONT, "pixel", true, {});
  ui.add("t", box.write("t.rml", page("font-family: pixel; font-size: 16px; padding-left: 3.3px;", "#H")));
  ui.show("t");
  Screen screen;
  screen.draw(ui);
  const auto [white, blended] = screen.count();
  CHECK(white > 200);
  CHECK(blended == 0);
}

TEST_CASE("ui text in a BMFont: whole texels, kerning applied") {
  Sandbox box;
  Ui ui{64, 32};
  const auto font = alpha::ui::load_font(ALPHA_TEST_FIXTURES "/fonts/tiny.fnt", "tiny", true, {});
  CHECK(font->native() == 4);
  ui.add("t", box.write("t.rml", page("font-family: tiny; font-size: 8px; line-height: 12px; padding-top: 4px;", "AV")));
  ui.show("t");
  Screen screen;
  screen.draw(ui);
  const auto [white, blended] = screen.count();
  CHECK(blended == 0);
  CHECK(white == 2 * 9 * 12);  // two 3x4 blocks at 3 output px per texel (8 px x 1.5 / 4)
  CHECK(ui.view()["t"].texts["t"] == "AV");
}

TEST_CASE("ui text with no font loaded uses the built-in font") {
  Sandbox box;
  Ui ui{64, 32};
  ui.add("t", box.write("t.rml", page("font-size: 8px;", "Hi")));
  ui.show("t");
  Screen screen;
  screen.draw(ui);
  const auto [white, blended] = screen.count();
  CHECK(white > 20);
  CHECK(blended == 0);
}
