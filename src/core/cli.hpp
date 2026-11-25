#pragma once

#include <span>
#include <string>
#include <string_view>

namespace alpha::core {

// What alpha_star was asked to do. Debug fields stay at their defaults in
// builds without ALPHA_DEBUG_TOOLS (their flags are not even compiled in).
struct CliOptions {
  std::string game;  // pack dir or .js; empty = the fused pack next to the exe
  int frames = 0;    // 0 = run until closed
  std::string screenshot;
  std::string tap;
  int tap_every = 0;
  bool devtools = false;
  std::string test_file;  // non-empty = test mode
  std::string test_assets;
  std::string perf;      // --perf PATH: per-section frame times as JSON at exit (bench)
  int control_port = 0;  // agent control channel (ALPHA_MCP); 0 = off
  bool hidden = false;   // no visible window (agent runs; ALPHA_MCP)
};

// Parses argv[1..]. Never throws: unknown flags, missing values and malformed
// numbers are ignored (a player double-clicking the exe must never see a crash).
CliOptions parse_cli(std::span<const std::string_view> args);

}  // namespace alpha::core
