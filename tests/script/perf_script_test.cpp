// Batch positions (ctx.positions), the profiler's step sections and soak runs
// that must keep memory bounded. Longer soak: ALPHA_SOAK_STEPS=216000 (1 hour at 60 fps).
#include <SDL3/SDL.h>
#include <doctest/doctest.h>

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

#include "host_fixture.hpp"
#include "script/perf_report.hpp"

using alpha::core::Section;
using alpha::script::Host;
using alpha::test::game_with;
using alpha::test::Sandbox;

namespace {

constexpr float kDt = 1.0f / 60.0f;

int soak_steps() {
  const char* v = SDL_getenv("ALPHA_SOAK_STEPS");
  return v && *v ? std::atoi(v) : 1200;
}

constexpr const char* kDot = "shape: { rect: [2, 2], color: '#fff' }";

}  // namespace

TEST_CASE("ctx.positions reads and writes every tagged position in one call each") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", std::string("enter(ctx) {"
      "  for (let i = 0; i < 3; i++) ctx.spawn({ pos: [i, 10 * i], tags: ['m'], ") + kDot + " });"
      "  ctx.spawn({ pos: [5, 5], tags: ['other'], " + kDot + " });"
      "  const p = ctx.positions('m');"
      "  check(p.count === 3 && p.x[2] === 2 && p.y[1] === 10, 'read');"
      "  for (let i = 0; i < p.count; i++) p.x[i] += 100;"
      "  ctx.destroy(ctx.findAll('m')[0]);"
      "  p.write();"
      "  const all = ctx.findAll('m');"
      "  check(all.length === 2 && all[0].pos.x === 101 && all[1].pos.x === 102 && all[1].pos.y === 20, 'written');"
      "  check(ctx.find('other').pos.x === 5, 'untouched');"
      "  check(ctx.positions('nobody').count === 0, 'unknown tag');"
      "}")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
}

TEST_CASE("a malformed positions buffer is a readable error") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", "enter(ctx) { globalThis.__native.setPositions(new ArrayBuffer(7)); }")));
  REQUIRE(host.paused());
  CHECK(host.error().find("must come from ctx.positions(tag)") != std::string::npos);
}

TEST_CASE("a step's time is split into script, timers, physics, animation and particles") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", "update(ctx) { let x = 0; for (let i = 0; i < 20000; i++) x += i; }")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  host.step({}, kDt);
  const alpha::core::FrameTimes t = host.frame_times();
  if constexpr (alpha::core::kProfile) CHECK(t[Section::script] > 0.0);
  CHECK(t[Section::timers] >= 0.0);
  CHECK(t[Section::physics] >= 0.0);
  host.end_frame();
  CHECK(host.perf().size() == 1);
  CHECK(host.frame_times().total() == 0.0);
}

TEST_CASE("--perf writes frame spreads, drawing, JS heap and entities as JSON") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", std::string("enter(ctx) { ctx.spawn({ pos: [1, 1], tags: ['a'], ") + kDot + " }); }")));
  host.step({}, kDt);
  host.end_frame();
  const std::string out = (box.dir / "perf.json").string();
  alpha::script::report_run(out, host, alpha::render::RenderStats{3, 1, 2, 12});
  std::ifstream in(out);
  const std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  CHECK(json.starts_with("{\"perf\":{\"frames\":1,\"total\":"));
  CHECK(json.find("\"drawCalls\":2,\"vertices\":12,\"drawn\":3,\"culled\":1") != std::string::npos);
  CHECK(json.find("\"entities\":1}") != std::string::npos);
  alpha::script::report_run("", host, {});  // no path: only the log line
  alpha::script::report_run((box.dir / "no" / "such" / "dir.json").string(), host, {});  // logged, no throw
}

TEST_CASE("soak: spawning and destroying for minutes keeps JS heap and entities bounded") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", game_with("", std::string("update(ctx) {"
      "  for (const e of ctx.findAll('p')) if (e.pos.y > 40) ctx.destroy(e); else e.pos.y += 1;"
      "  for (let i = 0; i < 5; i++) ctx.spawn({ pos: [i, 0], tags: ['p'], data: { n: i }, ") + kDot + " });"
      "  ctx.after(0.1, () => {});"
      "}")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const int steps = soak_steps();
  double early = 0.0;
  for (int i = 1; i <= steps && !host.paused(); ++i) {
    host.step({}, kDt);
    if (i == steps / 4) {
      host.collect_garbage();
      early = host.js_memory().objects;
    }
  }
  REQUIRE_MESSAGE(!host.paused(), host.error());
  host.collect_garbage();
  const double late = host.js_memory().objects;
  CHECK(host.entity_count() <= 5 * 42);
  CHECK_MESSAGE(late <= early * 1.05 + 50, "JS objects grew from " << early << " to " << late);
}
