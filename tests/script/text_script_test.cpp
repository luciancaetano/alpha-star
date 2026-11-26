#include <doctest/doctest.h>

#include <filesystem>
#include <string>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

std::string game(const std::string& enter, const std::string& assets = "{}") {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { enter(ctx) { " + enter + " } });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 3, actions: {} },\n"
         "  start: 'play', scenes: { play }, assets: " + assets + " });\n";
}

std::string error_of(const Sandbox& box, const std::string& enter, const std::string& assets = "{}") {
  Host host(box.save());
  host.load(box.write("g.js", game(enter, assets)));
  return host.error();
}

// assets.fonts: main (Latin) falling back to arabic, then cjk (Noto subsets).
std::string noto_assets(const Sandbox& box) {
  for (const char* f : {"NotoSans-Latin.ttf", "NotoSansArabic-Test.ttf", "NotoSansSC-Test.otf"}) {
    std::filesystem::copy_file(std::filesystem::path(ALPHA_TEST_FIXTURES "/fonts/noto") / f, box.dir / f,
                               std::filesystem::copy_options::overwrite_existing);
  }
  return "{ fonts: { main: { file: 'NotoSans-Latin.ttf', fallbacks: ['arabic', 'cjk'] },"
         " arabic: 'NotoSansArabic-Test.ttf', cjk: 'NotoSansSC-Test.otf' } }";
}

}  // namespace

TEST_CASE("world text refuses invalid Unicode with where and why") {
  Sandbox box;
  const std::string err = error_of(box, "ctx.spawn({ tags: [], pos: [0, 0], shape: { text: 'ok\\uD83D' } });");
  CHECK(err.find("shape.text is not valid Unicode text") != std::string::npos);
  CHECK(err.find("surrogate") != std::string::npos);
  CHECK(err.find("byte 2") != std::string::npos);
  CHECK(error_of(box, "ctx.spawn({ tags: [], pos: [0, 0], shape: { text: 'x' } }).text = '\\uDE00';")
            .find("entity.text is not valid Unicode text") != std::string::npos);
  CHECK(error_of(box, "ctx.spawn({ tags: [], pos: [0, 0], shape: { text: 'x', dir: 'up' } });")
            .find("\"auto\", \"ltr\" or \"rtl\"") != std::string::npos);
  CHECK(error_of(box, "ctx.spawn({ tags: [], pos: [0, 0], shape: { text: 'x', width: -1 } });")
            .find("shape.width must be 0") != std::string::npos);
}

TEST_CASE("world text in many scripts: fallbacks, dir and wrap width reach the layout") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game(R"(
    ctx.spawn({ tags: ['a'], pos: [50, 20], shape: { text: 'Hi سلام 你好', font: 'main', size: 16 } });
    ctx.spawn({ tags: ['w'], pos: [50, 60], shape: { text: '你好世界', font: 'main', size: 20, width: 45, dir: 'rtl' } });
  )", noto_assets(box))));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const auto items = host.world()->draw_items();
  REQUIRE(items.size() == 2);
  CHECK(items[1].shape.text->dir == alpha::core::TextDir::Rtl);
  CHECK(items[1].shape.text->width == 45.0f);
  const auto one = std::get<alpha::core::Rect>(items[0].shape.geometry).size;
  const auto two = std::get<alpha::core::Rect>(items[1].shape.geometry).size;
  CHECK(two.y > one.y * 2.0f * 0.9f);  // wrapped to two lines of 20 px text
  CHECK(two.x <= 45.0f);
  CHECK(error_of(box, "", "{ fonts: { a: { file: 'x.fnt', fallbacks: 'b' } } }").find("fallbacks must be an array") !=
        std::string::npos);
}
