#include "core/cli.hpp"

#include <charconv>

namespace alpha::core {

namespace {

#if ALPHA_DEBUG_TOOLS
// --compile <main.js> <out.jsbc>: the one flag with two values, only as the whole command.
bool compile_request(CliOptions& o, std::span<const std::string_view> args) {
  if (args.size() != 3 || args[0] != "--compile") return false;
  o.compile_source = args[1];
  o.compile_out = args[2];
  return true;
}
#else
bool compile_request(CliOptions&, std::span<const std::string_view>) { return false; }
#endif

#if ALPHA_DEBUG_TOOLS || ALPHA_MCP
// A non-negative int, or 0 when the text is not exactly one.
int count_of(std::string_view text) {
  int value = 0;
  const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  return ec == std::errc() && end == text.data() + text.size() && value > 0 ? value : 0;
}

// Each applies one `--flag value` of its feature; false when the flag is not theirs.
#if ALPHA_DEBUG_TOOLS
bool debug_option(CliOptions& o, std::string_view flag, std::string_view value) {
  if (flag == "--frames") o.frames = count_of(value);
  else if (flag == "--screenshot") o.screenshot = value;
  else if (flag == "--tap") o.tap = value;
  else if (flag == "--tap-every") o.tap_every = count_of(value);
  else if (flag == "--test") o.test_file = value;
  else if (flag == "--assets") o.test_assets = value;
  else if (flag == "--perf") o.perf = value;
  else if (flag == "--platform") o.platform = parse_os(value);
  else return false;
  return true;
}
#else
bool debug_option(CliOptions&, std::string_view, std::string_view) { return false; }
#endif

#if ALPHA_MCP
// A TCP port 1..65535, or 0 (off).
int port_of(std::string_view text) {
  const int port = count_of(text);
  return port <= 65535 ? port : 0;
}

bool control_option(CliOptions& o, std::string_view flag, std::string_view value) {
  if (flag != "--control") return false;
  o.control_port = port_of(value);
  return true;
}
#else
bool control_option(CliOptions&, std::string_view, std::string_view) { return false; }
#endif

// False when the flag takes no value or is unknown.
bool set_option(CliOptions& o, std::string_view flag, std::string_view value) {
  return debug_option(o, flag, value) || control_option(o, flag, value);
}

// Flags without a value; false when unknown.
bool set_switch(CliOptions& o, std::string_view flag) {
#if ALPHA_DEBUG_TOOLS
  if (flag == "--devtools") return o.devtools = true;
  if (flag == "--version") return o.version = true;
#endif
#if ALPHA_MCP
  if (flag == "--hidden") return o.hidden = true;
#endif
  return false;
}

void parse_flags(CliOptions& o, std::span<const std::string_view> flags) {
  for (std::size_t i = 0; i < flags.size(); ++i) {
    if (set_switch(o, flags[i])) continue;
    if (i + 1 < flags.size() && set_option(o, flags[i], flags[i + 1])) ++i;
  }
}
#else
// Player builds know no flags: every one is ignored.
void parse_flags(CliOptions&, std::span<const std::string_view>) {}
#endif

}  // namespace

CliOptions parse_cli(std::span<const std::string_view> args) {
  CliOptions o;
  if (compile_request(o, args)) return o;
  const bool has_game = !args.empty() && !args[0].starts_with('-');
  if (has_game) o.game = args[0];
  parse_flags(o, args.subspan(has_game ? 1 : 0));
  return o;
}

}  // namespace alpha::core
