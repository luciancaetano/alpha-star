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

// Filter of each drawn sprite, in spawn order: N(earest), L(inear), S(harp).
std::string flags(Host& host) {
  std::string out;
  for (const auto& item : host.world()->draw_items()) out += "NLS"[static_cast<int>(item.shape.image->filter)];
  return out;
}

}  // namespace

TEST_CASE("image filter: config.pixelMode default, per-image and per-sheet overrides") {
  Sandbox box;
  write_bmp(box, "p.bmp");
  const std::string assets =
      "{ images: { crisp: 'p.bmp', soft: { file: 'p.bmp', filter: 'linear' } }, sheets: { run: { image: 'p.bmp', frame: [8, 8], filter: 'sharp' } } }";
  Host pixel(box.save());
  pixel.load(box.write("g.js", game("", assets)));
  REQUIRE_MESSAGE(!pixel.paused(), pixel.error());
  CHECK(flags(pixel) == "NLS");

  const std::string overrides =
      "{ images: { crisp: { file: 'p.bmp', filter: 'sharp' }, soft: 'p.bmp' }, sheets: { run: { image: 'p.bmp', frame: [8, 8], filter: 'nearest' } } }";
  Host smooth(box.save());
  smooth.load(box.write("h.js", game(", pixelMode: false", overrides)));
  REQUIRE_MESSAGE(!smooth.paused(), smooth.error());
  CHECK(flags(smooth) == "SLN");

  Host bad(box.save());
  bad.load(box.write("b.js", game("", "{ images: { crisp: { file: 'p.bmp', filter: 'blur' } } }")));
  CHECK(bad.error().find("assets.images.crisp.filter must be \"nearest\" (crisp), \"linear\" (smooth) or \"sharp\"") != std::string::npos);
  Host old(box.save());
  old.load(box.write("o.js", game("", "{ images: { crisp: { file: 'p.bmp', pixelMode: true } } }")));
  CHECK(old.error().find("assets.images.crisp.pixelMode is not an image option; use filter:") != std::string::npos);
  Host missing(box.save());
  missing.load(box.write("m.js", game("", "{ images: { crisp: { path: 'p.bmp' } } }")));
  CHECK(missing.error().find("assets.images.crisp must be a file path or { file, filter? }") != std::string::npos);
}

namespace {

// Three filled shapes: no override, smooth: true, smooth: false.
std::string shapes_game(const std::string& pixel_config, const std::string& extra = "") {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { enter(ctx) {\n"
         "  ctx.spawn({ tags: ['a'], pos: [0, 0], shape: { circle: 4, color: '#fff' } });\n"
         "  ctx.spawn({ tags: ['b'], pos: [0, 0], shape: { rect: [4, 4], color: '#fff', smooth: true } });\n"
         "  ctx.spawn({ tags: ['c'], pos: [0, 0], shape: { circle: 4, color: '#fff', smooth: false" + extra + " } });\n"
         "} });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 1, actions: {} " + pixel_config + " },\n"
         "  start: 'play', scenes: { play } });\n";
}

std::string shape_flags(Host& host) {
  std::string out;
  for (const auto& item : host.world()->draw_items()) out += item.shape.smooth ? "S" : "P";
  return out;
}

}  // namespace

TEST_CASE("pixelMode: filled shapes are crisp by default, smooth with pixelMode false, shape.smooth overrides") {
  Sandbox box;
  Host pixel(box.save());
  pixel.load(box.write("g.js", shapes_game("")));
  REQUIRE_MESSAGE(!pixel.paused(), pixel.error());
  CHECK(shape_flags(pixel) == "PSP");

  Host smooth(box.save());
  smooth.load(box.write("h.js", shapes_game(", pixelMode: false")));
  REQUIRE_MESSAGE(!smooth.paused(), smooth.error());
  CHECK(shape_flags(smooth) == "SSP");

  Host bad(box.save());
  bad.load(box.write("b.js", shapes_game("", ", smooth: 1")));
  CHECK(bad.error().find("shape.smooth must be true (antialiased edges) or false (crisp), got 1") != std::string::npos);
}
