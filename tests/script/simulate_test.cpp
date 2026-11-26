#include <doctest/doctest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::script::TestResult;
using alpha::test::Sandbox;

namespace {

// A game: `play` goes to `over` after 1 s; `jump` spawns an "x"; `boom` throws.
constexpr const char* kGame = R"(
import { define } from 'alpha'; const { scene, game } = define();
const play = scene('play', {
  enter(ctx) {
    ctx.after(1, () => ctx.go('over'));
    for (let i = ctx.random.int(1, 50); i > 0; --i) ctx.spawn({ tags: ['r'], pos: [0, 0] });
    if (ctx.save.get('seen', false)) ctx.go('over');
    ctx.save.set('seen', true);
  },
  update(ctx) { if (ctx.input.pressed('jump')) ctx.spawn({ tags: ['x'], pos: [0, 0] }); },
});
const over = scene('over', {});
const boom = scene('boom', { update(ctx) { if (ctx.time > 0.1) null.crash(); } });
export default game({ config: { title: 't', size: [100, 100], seed: 7, actions: { jump: ['Space'] } },
  start: 'play', scenes: { play, over, boom } });
)";

std::vector<TestResult> run(const Sandbox& box, const std::string& tests) {
  // Absolute import: real test bundles inline the game; this one keeps it separate.
  const std::string game = std::filesystem::path(box.write("game.js", kGame)).generic_string();
  Host host(box.save());
  const auto results = host.test(box.write("tests.js", "import { simulate } from 'alpha/test';\n"
                                                       "import game from '" + game + "';\n" + tests));
  CHECK_FALSE(std::filesystem::exists(box.save_file()));  // simulate never writes the real save
  return results;
}

std::string only_error(const Sandbox& box, const std::string& tests) {
  const auto results = run(box, tests);
  REQUIRE(results.size() == 1);
  return results[0].error;
}

}  // namespace

TEST_CASE("the test runner runs every exported function and reports each by name") {
  Sandbox box;
  const auto results = run(box, "export function good() {}\nexport function bad() { throw new Error('nope'); }\n"
                                "export const notATest = 1;\n");
  REQUIRE(results.size() == 2);
  CHECK(results[0].name == "bad");
  CHECK(results[0].error.find("nope") != std::string::npos);
  CHECK(results[1].name == "good");
  CHECK(results[1].passed());
}

TEST_CASE("a test module that fails to load is one failing result") {
  Sandbox box;
  const auto results = run(box, "export function t() {}\nthrow new Error('broken module');\n");
  REQUIRE(results.size() == 1);
  CHECK_FALSE(results[0].passed());
  CHECK(results[0].error.find("broken module") != std::string::npos);
}

TEST_CASE("simulate runs fixed 1/60 steps and expect.scene checks where the game ended") {
  Sandbox box;
  CHECK(only_error(box, "export function t() {\n"
                        "  const r = simulate(game, { seconds: 1.1 });\n"
                        "  if (r.frames !== 66) throw new Error('frames ' + r.frames);\n"
                        "  r.expect.scene('over'); }\n") == "");
  CHECK(only_error(box, "export function t() { simulate(game, { seconds: 0.5 }).expect.scene('over'); }")
            .find("expected scene \"over\" after 30 frames, got \"play\"") != std::string::npos);
}

TEST_CASE("simulate presses actions on the given frames") {
  Sandbox box;
  CHECK(only_error(box, "export function t() {\n"
                        "  simulate(game, { seconds: 0.5, inputs: { jump: [3, 10] } }).expect.count('x').equals(2);\n"
                        "  simulate(game, { seconds: 0.5 }).expect.count('x').equals(0); }\n") == "");
  CHECK(only_error(box, "export function t() { simulate(game, { seconds: 0.5 }).expect.count('x').atLeast(1); }")
            .find("expected at least 1 \"x\" after 30 frames, got 0") != std::string::npos);
  CHECK(only_error(box, "export function t() { simulate(game, { seconds: 1, inputs: { jmp: [1] } }); }")
            .find("inputs.jmp is not an action; actions: jump") != std::string::npos);
}

TEST_CASE("every simulate starts fresh: same seed, empty in-memory save, chosen scene") {
  Sandbox box;
  CHECK(only_error(box, "export function t() {\n"
                        "  const a = simulate(game, { seconds: 0.1 }), b = simulate(game, { seconds: 0.1 });\n"
                        "  a.expect.scene('play'); b.expect.scene('play');\n"
                        "  for (let n = 1; n <= 50; ++n) {\n"
                        "    try { a.expect.count('r').equals(n); } catch { continue; }\n"
                        "    b.expect.count('r').equals(n); return; }\n"
                        "  throw new Error('no count matched'); }\n") == "");
  CHECK(only_error(box, "export function t() { simulate(game, { scene: 'over', seconds: 0.1 }).expect.scene('over'); }") ==
        "");
}

TEST_CASE("a script error inside simulate stops that run and fails noErrors with the message") {
  Sandbox box;
  const std::string error =
      only_error(box, "export function t() { simulate(game, { scene: 'boom', seconds: 1 }).expect.noErrors(); }");
  CHECK(error.find("expected no errors, got") != std::string::npos);
  CHECK(error.find("cannot read property") != std::string::npos);
  CHECK(only_error(box, "export function t() { simulate(game, { scene: 'nope', seconds: 1 }); }")
            .find("scene \"nope\" is not a scene; scenes: play, over, boom") != std::string::npos);
}

namespace {
// Inline source map: every column of generated line 2 -> src/play.ts:5:1.
constexpr const char* kMap =
    "\n//# sourceMappingURL=data:application/json;base64,"
    "eyJ2ZXJzaW9uIjozLCJzb3VyY2VzIjpbIi4uL3NyYy9wbGF5LnRzIl0sIm1hcHBpbmdzIjoiO0FBSUEifQ==\n";
}  // namespace

TEST_CASE("a paused game names the TypeScript source through the bundle's source map") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("main.js", alpha::test::tiny("update", "null.crash();") + kMap));
  host.step({}, 1.0f / 60.0f);
  REQUIRE(host.paused());
  CHECK(host.error().find("src/play.ts:5:1") != std::string::npos);
  CHECK(host.error().find("main.js:2") == std::string::npos);
}

TEST_CASE("test failures name the TypeScript source too") {
  Sandbox box;
  Host host(box.save());
  const auto results = host.test(box.write("tests.js", std::string("// tests\nexport function t() { null.crash(); }") + kMap));
  REQUIRE(results.size() == 1);
  CHECK(results[0].error.find("src/play.ts:5:1") != std::string::npos);
}

TEST_CASE("simulate size: the game sees the world config.window.aspect shows on that screen") {
  Sandbox box;
  const auto game_with = [&box](const std::string& name, const std::string& window) {
    return std::filesystem::path(box.write(name, "import { define } from 'alpha'; const { scene, game } = define();\n"
                                                 "const play = scene('play', { state: () => ({}), enter(ctx) { ctx.state.view = ctx.camera.view;"
                                                 " ctx.state.safe = ctx.safeArea; } });\n"
                                                 "export default game({ config: { title: 't', size: [288, 512]" + window +
                                                     " }, start: 'play', scenes: { play } });\n"))
        .generic_string();
  };
  const std::string fill = game_with("fill.js", "");
  const std::string bars = game_with("bars.js", ", window: { aspect: 'keep' }");
  Host host(box.save());
  const auto results = host.test(box.write(
      "tests.js", "import { simulate } from 'alpha/test';\nimport fill from '" + fill + "';\nimport bars from '" + bars + "';\n"
                  "const view = (g, size) => JSON.stringify(simulate(g, { seconds: 0.1, size }).state);\n"
                  "export function a() { const v = view(fill, [1080, 2340]);\n"
                  "  if (v !== JSON.stringify({ view: [0, -56, 288, 624], safe: [0, 0, 288, 624] })) throw new Error(v); }\n"
                  "export function b() { const v = view(bars, [1080, 2340]);\n"
                  "  if (v !== JSON.stringify({ view: [0, 0, 288, 512], safe: [0, 0, 288, 512] })) throw new Error(v); }\n"
                  "export function c() { simulate(fill, { seconds: 0.1, size: [0, 5] }); }\n"));
  REQUIRE(results.size() == 3);
  CHECK_MESSAGE(results[0].passed(), results[0].error);
  CHECK_MESSAGE(results[1].passed(), results[1].error);
  CHECK(results[2].error.find("size must be [width, height] screen pixels") != std::string::npos);
}
