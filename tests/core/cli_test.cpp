#include <doctest/doctest.h>

#include <string_view>
#include <vector>

#include "core/cli.hpp"

using alpha::core::CliOptions;
using alpha::core::parse_cli;

namespace {

CliOptions parse(std::vector<std::string_view> args) { return parse_cli(args); }

}  // namespace

TEST_CASE("cli: the first argument is the game unless it is a flag") {
  CHECK(parse({"pack"}).game == "pack");
  CHECK(parse({}).game.empty());
  CHECK(parse({"--bogus"}).game.empty());
}

TEST_CASE("cli: unknown flags and their values are ignored safely") {
  const CliOptions o = parse({"pack", "--bogus", "3", "--also-unknown"});
  CHECK(o.game == "pack");
  CHECK(o.frames == 0);
  CHECK_FALSE(o.devtools);
  CHECK(o.test_file.empty());
}

#if ALPHA_DEBUG_TOOLS
TEST_CASE("cli: --compile <main.js> <out.jsbc> is a whole command") {
  const CliOptions o = parse({"--compile", "dist/main.js", "dist/game.jsbc"});
  CHECK(o.compile_source == "dist/main.js");
  CHECK(o.compile_out == "dist/game.jsbc");
  CHECK(o.game.empty());
  CHECK(parse({"--compile", "main.js"}).compile_source.empty());
  CHECK(parse({"pack", "--compile", "a.js", "b.jsbc"}).compile_source.empty());
}

TEST_CASE("cli: --version (the SDK checks its engines report its own version)") {
  CHECK(parse({"--version"}).version);
  CHECK(parse({"--version"}).game.empty());
  CHECK_FALSE(parse({"pack"}).version);
}

TEST_CASE("cli: debug flags (ALPHA_DEBUG_TOOLS builds)") {
  const CliOptions o = parse({"pack", "--frames", "120", "--screenshot", "out.bmp", "--tap", "Space",
                              "--tap-every", "20", "--devtools"});
  CHECK(o.frames == 120);
  CHECK(o.screenshot == "out.bmp");
  CHECK(o.tap == "Space");
  CHECK(o.tap_every == 20);
  CHECK(o.devtools);
}

TEST_CASE("cli: a malformed number is ignored, never a crash") {
  CHECK(parse({"pack", "--frames", "abc"}).frames == 0);
  CHECK(parse({"pack", "--frames", "-5"}).frames == 0);
  CHECK(parse({"pack", "--frames"}).frames == 0);
}

TEST_CASE("cli: test mode") {
  const CliOptions o = parse({"--test", "t.js", "--assets", "proj"});
  CHECK(parse({"pack", "--frames", "9", "--perf", "p.json"}).perf == "p.json");
  CHECK(o.game.empty());
  CHECK(o.test_file == "t.js");
  CHECK(o.test_assets == "proj");
}

TEST_CASE("cli: --platform previews another OS's UI (ctx.platform); unknown names are ignored") {
  CHECK(parse({"pack", "--platform", "android"}).platform == alpha::core::Os::android);
  CHECK_FALSE(parse({"pack", "--platform", "Android"}).platform.has_value());
  CHECK_FALSE(parse({"pack"}).platform.has_value());
}
#else
TEST_CASE("cli: without ALPHA_DEBUG_TOOLS every flag is unknown") {
  const CliOptions o = parse({"pack", "--frames", "120", "--test", "t.js"});
  CHECK(parse({"--compile", "a.js", "b.jsbc"}).compile_source.empty());
  CHECK(o.frames == 0);
  CHECK(o.test_file.empty());
}
#endif

#if ALPHA_MCP
TEST_CASE("cli: agent control flags (ALPHA_MCP builds)") {
  const CliOptions o = parse({"pack", "--control", "4321", "--hidden"});
  CHECK(o.control_port == 4321);
  CHECK(o.hidden);
  CHECK(parse({"pack", "--control", "70000"}).control_port == 0);
  CHECK(parse({"pack", "--control", "x"}).control_port == 0);
  CHECK_FALSE(parse({"pack"}).hidden);
}
#else
TEST_CASE("cli: without ALPHA_MCP the control flags are unknown") {
  const CliOptions o = parse({"pack", "--control", "4321", "--hidden"});
  CHECK(o.control_port == 0);
  CHECK_FALSE(o.hidden);
}
#endif

TEST_CASE("cli: hot reload watches only a pack named on the command line") {
  CHECK(alpha::core::watches_pack(parse({"pack"})));
  // Bundled pack (Android APK, fused exe): its dir may hold the saves; a save must never reload the game.
  CHECK_FALSE(alpha::core::watches_pack(parse({})));
}
