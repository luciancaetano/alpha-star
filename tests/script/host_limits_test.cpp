#include <doctest/doctest.h>

#include <string>

#include "host_fixture.hpp"

using alpha::script::Host;
using alpha::test::Sandbox;
using alpha::test::tiny;
namespace fs = std::filesystem;

namespace {
constexpr float kDt = 1.0f / 60.0f;
}  // namespace

TEST_CASE("an endless loop pauses the game with the time budget instead of freezing") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("loop.js", tiny("update", "while (true) {}")));
  REQUIRE_FALSE(host.paused());
  host.step({}, kDt);
  REQUIRE(host.paused());
  CHECK(host.error().find("script ran longer than 250 ms") != std::string::npos);
  CHECK(host.error().find("infinite loop?") != std::string::npos);
}

TEST_CASE("an allocation bomb pauses the game with the memory limit") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("bomb.js", tiny("update", "const a = []; for (;;) a.push(new Array(1e6).fill(1));")));
  host.step({}, kDt);
  REQUIRE(host.paused());
  CHECK(host.error().find("out of memory") != std::string::npos);
  CHECK(host.error().find("256 MB") != std::string::npos);
}

TEST_CASE("unbounded recursion inside a native call is a stack overflow error, not a crash") {
  Sandbox box;
  Host host(box.save());
  // String() of an array holding itself recurses through Array.join from C++ (a native's error message).
  host.load(box.write("deep.js", tiny("update", "const a = [1]; a.push(a); globalThis.__native.loadSound(a, 'x.wav', 'sfx');")));
  host.step({}, kDt);
  REQUIRE(host.paused());
  CHECK(host.error().find("sound name must be a string") != std::string::npos);
}

TEST_CASE("plain unbounded recursion pauses the game with a stack overflow error") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("deep.js", tiny("update", "const f = (n) => [f(n + 1)]; f(0);")));
  host.step({}, kDt);
  REQUIRE(host.paused());
  CHECK_MESSAGE(host.error().find("Maximum call stack size exceeded") != std::string::npos, host.error());
}

TEST_CASE("the save file is chosen from the game title") {
  Sandbox box;
  std::string seen;
  Host host([&](const std::string& title) {
    seen = title;
    return box.save_file();
  });
  host.load(box.write("t.js", tiny("enter", "ctx.save.set('k', 1);")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  CHECK(seen == "t");
  CHECK(fs::exists(box.save_file()));
}
