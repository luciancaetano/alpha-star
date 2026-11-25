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
  CHECK(o.game.empty());
  CHECK(o.test_file == "t.js");
  CHECK(o.test_assets == "proj");
}
#else
TEST_CASE("cli: without ALPHA_DEBUG_TOOLS every flag is unknown") {
  const CliOptions o = parse({"pack", "--frames", "120", "--test", "t.js"});
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
