#include <doctest/doctest.h>

#include "host_fixture.hpp"

using alpha::core::LogLevel;
using alpha::script::Host;
using alpha::test::Sandbox;
using alpha::test::tiny;

TEST_CASE("console.log/warn/error reach the host log, arguments joined like a browser") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", tiny("enter",
      "console.log('hi', 1, { a: 2 }, [3]); console.warn('careful'); console.error(null, undefined);")));
  const auto& log = host.log();
  REQUIRE(log.size() == 3);
  CHECK(log.at(0).level == LogLevel::Info);
  CHECK(log.at(0).text == "hi 1 {\"a\":2} [3]");
  CHECK(log.at(1).level == LogLevel::Warn);
  CHECK(log.at(1).text == "careful");
  CHECK(log.at(2).level == LogLevel::Error);
  CHECK(log.at(2).text == "null undefined");
  CHECK_FALSE(host.paused());
}

TEST_CASE("a script error that pauses the game is also logged") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", tiny("update", "throw new Error('kaput');")));
  host.step({}, 1.0f / 60.0f);
  REQUIRE(host.paused());
  REQUIRE(host.log().size() == 1);
  CHECK(host.log().at(0).level == LogLevel::Error);
  CHECK(host.log().at(0).text == host.error());
}

TEST_CASE("host exposes the scene, the world and the last script call time") {
  Sandbox box;
  Host host(box.save());
  CHECK(host.scene().empty());
  CHECK(host.world() == nullptr);
  host.load(box.write("g.js", tiny("enter", "ctx.spawn({ tags: [], pos: [0, 0] });")));
  CHECK(host.scene() == "play");
  REQUIRE(host.world() != nullptr);
  CHECK(host.world()->entities().size() == 1);
  CHECK(host.tags().size() == 0);
  CHECK(host.last_call_ms() >= 0.0);
}
