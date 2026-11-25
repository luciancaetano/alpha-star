// Fuzzing the script boundary: seeded random and malformed values thrown at
// every native (__native.*) and every ctx function, including nested spec
// objects (spawn, emitter, body, joint, synth, UI, saves). The engine must
// never crash or hang; every bad input must surface as a readable Error.
// The fuzzer game is tests/fixtures/fuzz/fuzz_game.js.
//
// Bounded in the gate (seconds). Longer opt-in run, always under the resource guard:
//   ALPHA_FUZZ_SEEDS=50 ALPHA_FUZZ_STEPS=200 python tools/resource_guard.py -- ctest --preset dev -R fuzz
// ALPHA_FUZZ_SEED picks the first seed, ALPHA_FUZZ_ONLY narrows targets (regex over
// names like "N.spawn" / "ctx.audio.play"), ALPHA_FUZZ_TRACE=1 logs each call.
#include <SDL3/SDL.h>
#include <doctest/doctest.h>

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

int env_int(const char* name, int fallback) {
  const char* v = SDL_getenv(name);
  return v && *v ? std::atoi(v) : fallback;
}

std::string env_text(const char* name, const char* fallback) {
  const char* v = SDL_getenv(name);
  return v && *v ? v : fallback;
}

void fill(std::string& text, const std::string& key, const std::string& value) {
  for (auto at = text.find(key); at != std::string::npos; at = text.find(key, at + value.size())) {
    text.replace(at, key.size(), value);
  }
}

// The fuzzer game for one seed: `calls` random calls per step.
std::string fuzz_game(int seed, int calls) {
  std::ifstream in(std::string(ALPHA_TEST_FIXTURES) + "/fuzz/fuzz_game.js");
  std::stringstream source;
  source << in.rdbuf();
  std::string text = source.str();
  fill(text, "__SEED__", std::to_string(seed));
  fill(text, "__CALLS__", std::to_string(calls));
  fill(text, "__ONLY__", env_text("ALPHA_FUZZ_ONLY", "."));
  fill(text, "__TRACE__", SDL_getenv("ALPHA_FUZZ_TRACE") ? "true" : "false");
  return text;
}

bool logged(const Host& host, const std::string& needle) {
  const alpha::core::LogRing& log = host.log();
  for (std::size_t i = 0; i < log.size(); ++i) {
    if (log.at(i).text.find(needle) != std::string::npos) return true;
  }
  return false;
}

// Readable: an Error with its message, or an engine limit (time budget, memory) explained.
bool readable(const std::string& error) {
  return error.find("Error") != std::string::npos || error.find("script ran") != std::string::npos;
}

// One fresh game fuzzed until it pauses or `steps` is reached; returns the steps it ran.
int fuzz_run(const Sandbox& box, int seed, int steps) {
  Host host(box.save());
  host.load(box.write("fuzz.js", fuzz_game(seed, 60)));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  int done = 0;
  while (done < steps && !host.paused()) {
    host.step({}, 1.0f / 60.0f);
    (void)host.frame();
    ++done;
  }
  if (host.paused()) {
    CHECK_MESSAGE(readable(host.error()), host.error());
    if (SDL_getenv("ALPHA_FUZZ_TRACE")) MESSAGE(host.error());
  } else {
    CHECK(logged(host, "FUZZ-DONE"));
  }
  CHECK_FALSE(logged(host, "FUZZ-BAD"));
  return done;
}

}  // namespace

TEST_CASE("absurd sizes are refused with readable errors before any native allocation") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("caps.js", alpha::test::game_with("", R"JS(enter(ctx) {
    const N = globalThis.__native;
    const refused = (fn, text) => {
      try { fn(); } catch (e) { check(e.message.includes(text), text + ' in: ' + e.message); return; }
      throw new Error('not refused: ' + text);
    };
    const huge = ['x'];
    huge.length = 4294967295;  // sparse: no JS memory, but a native loop over it would never end
    refused(() => ctx.spawn({ pos: [0, 0], tags: huge }), 'at most 1048576');
    refused(() => ctx.spawn({ pos: [0, 0], tags: [], shape: { text: 'x'.repeat(5000) } }), 'displayed text is at most 4096');
    const label = ctx.spawn({ pos: [0, 0], tags: [], shape: { text: 'ok' } });
    refused(() => { label.text = 'y'.repeat(4097); }, 'at most 4096');
    const note = { wave: 'square', freq: 440, duration: 0.1 };
    refused(() => N.synthSound('s', new Array(300).fill(note), 'sfx'), 'at most 256');
    for (let i = 0; i < 30; i++) {
      ctx.spawn({ pos: [i, 0], tags: ['fx'], emitter: { rate: 20000, max: 20000, lifetime: [5, 5] } });
    }
    refused(() => ctx.findAll('fx')[0].emit(1e12), 'count must be 0..20000');
    refused(() => { for (let i = 0; i < 100; i++) N.instanceCreate(); }, 'at most 64 game instances');
    refused(() => ctx.every(5e-324, () => {}), '>= 0.001');
  },
  update(ctx) {
    let live = 0;
    for (const e of ctx.findAll('fx')) live += e.particles;
    check(live <= 20000, 'world particles capped, got ' + live);
  })JS")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  for (int i = 0; i < 5; ++i) host.step({}, 1.0f / 60.0f);
  CHECK_MESSAGE(!host.paused(), host.error());
}

TEST_CASE("a native that throws frees the JS values it made (no leak abort at shutdown)") {
  Sandbox box;
  {
    Host host(box.save());
    // An instance without a UI: uiView throws; it used to throw after creating its result object.
    host.load(box.write("leak.js", alpha::test::game_with("", R"JS(enter(ctx) {
      const N = globalThis.__native;
      const id = N.instanceCreate();
      N.instanceUse(id);
      let msg = '';
      try { N.uiView(); } catch (e) { msg = e.message; }
      N.instanceUse(0);
      N.instanceDestroy(id);
      check(msg.includes('the game UI exists only after boot'), msg);
    })JS")));
    REQUIRE_MESSAGE(!host.paused(), host.error());
  }  // QuickJS asserts here (Debug) when a native leaked an object
  CHECK(true);
}

TEST_CASE("fuzzing every native and ctx function never crashes the engine") {
  const int seeds = env_int("ALPHA_FUZZ_SEEDS", 3);
  const int steps = env_int("ALPHA_FUZZ_STEPS", 10);
  const int first = env_int("ALPHA_FUZZ_SEED", 1);
  Sandbox box;
  for (int seed = first; seed < first + seeds; ++seed) {
    // A fuzzed call may break the runtime and pause the game: a fresh game (next sub-seed) goes on.
    for (int done = 0, run = 0; done < steps; ++run) {
      CAPTURE(seed);
      CAPTURE(run);
      done += fuzz_run(box, seed * 1000 + run, steps - done);
    }
  }
}
