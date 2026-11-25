#include "core/cli.hpp"

#include <charconv>

namespace alpha::core {

namespace {

#if ALPHA_DEBUG_TOOLS
// A non-negative int, or 0 when the text is not exactly one.
int count_of(std::string_view text) {
  int value = 0;
  const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
  return ec == std::errc() && end == text.data() + text.size() && value > 0 ? value : 0;
}

// Applies one `--flag value`; false when the flag takes no value or is unknown.
bool set_option(CliOptions& o, std::string_view flag, std::string_view value) {
  if (flag == "--frames") o.frames = count_of(value);
  else if (flag == "--screenshot") o.screenshot = value;
  else if (flag == "--tap") o.tap = value;
  else if (flag == "--tap-every") o.tap_every = count_of(value);
  else if (flag == "--test") o.test_file = value;
  else if (flag == "--assets") o.test_assets = value;
  else return false;
  return true;
}

void parse_flags(CliOptions& o, std::span<const std::string_view> flags) {
  for (std::size_t i = 0; i < flags.size(); ++i) {
    if (flags[i] == "--devtools") o.devtools = true;
    else if (i + 1 < flags.size() && set_option(o, flags[i], flags[i + 1])) ++i;
  }
}
#else
// Player builds know no flags: every one is ignored.
void parse_flags(CliOptions&, std::span<const std::string_view>) {}
#endif

}  // namespace

CliOptions parse_cli(std::span<const std::string_view> args) {
  CliOptions o;
  const bool has_game = !args.empty() && !args[0].starts_with('-');
  if (has_game) o.game = args[0];
  parse_flags(o, args.subspan(has_game ? 1 : 0));
  return o;
}

}  // namespace alpha::core
