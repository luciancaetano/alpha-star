#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <string>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

void write_bmp(const Sandbox& box, const char* name) {
  SDL_Surface* px = SDL_CreateSurface(16, 8, SDL_PIXELFORMAT_RGBA32);
  SDL_SaveBMP(px, (box.dir / name).string().c_str());
  SDL_DestroySurface(px);
}

// A game spawning one sprite per image (crisp, soft) and one sheet sprite.
std::string game(const std::string& pixel_config, const std::string& assets) {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { enter(ctx) {\n"
         "  ctx.spawn({ tags: ['a'], pos: [0, 0], shape: { image: 'crisp' } });\n"
         "  ctx.spawn({ tags: ['b'], pos: [0, 0], shape: { image: 'soft' } });\n"
         "  ctx.spawn({ tags: ['c'], pos: [0, 0], shape: { sheet: 'run' } });\n"
         "} });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 1, actions: {} " + pixel_config + " },\n"
         "  start: 'play', scenes: { play }, assets: " + assets + " });\n";
}

// smooth flag of each drawn sprite, in spawn order.
std::string flags(Host& host) {
  std::string out;
  for (const auto& item : host.world()->draw_items()) out += item.shape.image->smooth ? "S" : "P";
  return out;
}

}  // namespace

TEST_CASE("pixelMode: config default, per-image and per-sheet overrides") {
  Sandbox box;
  write_bmp(box, "p.bmp");
  const std::string assets =
      "{ images: { crisp: 'p.bmp', soft: { file: 'p.bmp', pixelMode: false } }, sheets: { run: { image: 'p.bmp', frame: [8, 8] } } }";
  Host pixel(box.save());
  pixel.load(box.write("g.js", game("", assets)));
  REQUIRE_MESSAGE(!pixel.paused(), pixel.error());
  CHECK(flags(pixel) == "PSP");

  const std::string overrides =
      "{ images: { crisp: { file: 'p.bmp', pixelMode: true }, soft: 'p.bmp' }, sheets: { run: { image: 'p.bmp', frame: [8, 8], pixelMode: true } } }";
  Host smooth(box.save());
  smooth.load(box.write("h.js", game(", pixelMode: false", overrides)));
  REQUIRE_MESSAGE(!smooth.paused(), smooth.error());
  CHECK(flags(smooth) == "PSP");

  Host bad(box.save());
  bad.load(box.write("b.js", game("", "{ images: { crisp: { file: 'p.bmp', pixelMode: 'yes' } } }")));
  CHECK(bad.error().find("assets.images.crisp.pixelMode must be true (crisp pixel art) or false (smooth)") != std::string::npos);
  Host missing(box.save());
  missing.load(box.write("m.js", game("", "{ images: { crisp: { path: 'p.bmp' } } }")));
  CHECK(missing.error().find("assets.images.crisp must be a file path or { file, pixelMode? }") != std::string::npos);
}
