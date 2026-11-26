#include <doctest/doctest.h>

#include "devtools/snapshot.hpp"
#include "host_fixture.hpp"

using alpha::devtools::FrameStats;
using alpha::devtools::snapshot;
using alpha::script::Host;
using alpha::test::Sandbox;
using alpha::test::tiny;

TEST_CASE("snapshot reports frame rate, steps, scene, entities, voices and the call budget") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", tiny("enter", "ctx.spawn({ tags: ['a'], pos: [0, 0] }); ctx.spawn({ tags: [], pos: [1, 1] });")));
  host.step({}, 1.0f / 60.0f);
  const auto s = snapshot(host, FrameStats{20.0, 2, {}});
  CHECK(s.fps == doctest::Approx(50.0));
  CHECK(s.frame_ms == doctest::Approx(20.0));
  CHECK(s.steps == 2);
  CHECK(s.scene == "play");
  CHECK(s.entities == 2);
  CHECK(s.voices == 0);
  CHECK(s.budget_ms == doctest::Approx(250.0));
  CHECK(s.call_ms >= 0.0);
  CHECK(s.call_ms < 250.0);
  CHECK_MESSAGE(s.error.empty(), s.error);
}

TEST_CASE("snapshot of a game that failed to boot has no scene and no entities") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", "throw new Error('boom');"));
  const auto s = snapshot(host, FrameStats{0.0, 0, {}});
  CHECK(s.fps == doctest::Approx(0.0));
  CHECK(s.entities == 0);
  CHECK(s.scene.empty());
  CHECK(s.error.find("boom") != std::string::npos);
}
