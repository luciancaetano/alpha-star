#include "core/source_map.hpp"

#include <algorithm>
#include <charconv>

#include "core/error.hpp"

namespace alpha::core {

namespace {

constexpr std::string_view kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr std::string_view kDataUrl = "//# sourceMappingURL=data:application/json;base64,";

int digit_of(char c) {
  const std::size_t at = kAlphabet.find(c);
  if (at == std::string_view::npos) throw Error(std::string("source map: invalid base64 character '") + c + "'");
  return static_cast<int>(at);
}

std::string strip_relative(std::string name) {
  while (name.starts_with("../") || name.starts_with("./")) name.erase(0, name.find('/') + 1);
  return name;
}

// Digits at text[at...]; advances `at`. nullopt if there are none.
std::optional<int> read_int(std::string_view text, std::size_t& at) {
  int value = 0;
  const auto [end, ec] = std::from_chars(text.data() + at, text.data() + text.size(), value);
  if (ec != std::errc{}) return std::nullopt;
  at = static_cast<std::size_t>(end - text.data());
  return value;
}

struct Location {
  int line;
  int column;  // 0 when the text had no column
  std::size_t end;
};

// Parses ":LINE[:COL]" at text[at]; nullopt if it is not there.
std::optional<Location> location_at(std::string_view text, std::size_t at) {
  if (at >= text.size() || text[at] != ':') return std::nullopt;
  ++at;
  const auto line = read_int(text, at);
  if (!line) return std::nullopt;
  std::size_t after_col = at + 1;
  const auto column = at < text.size() && text[at] == ':' ? read_int(text, after_col) : std::nullopt;
  return Location{*line, column.value_or(0), column ? after_col : at};
}

}  // namespace

SourceMap::SourceMap(std::string_view mappings, std::vector<std::string> sources) {
  for (std::string& s : sources) sources_.push_back(strip_relative(std::move(s)));
  Segment prev{0, 0, 0, 0};
  std::size_t line = 0;
  std::size_t start = 0;
  for (std::size_t i = 0; i <= mappings.size(); ++i) {
    if (i < mappings.size() && mappings[i] != ',' && mappings[i] != ';') continue;
    if (i > start) add_segment(line, decode_vlq(mappings.substr(start, i - start)), prev);
    start = i + 1;
    if (i < mappings.size() && mappings[i] == ';') {
      ++line;
      prev.column = 0;
    }
  }
}

void SourceMap::add_segment(std::size_t line, const std::vector<int>& fields, Segment& prev) {
  prev.column += fields[0];
  if (fields.size() < 4) return;  // a segment with no source maps nothing
  prev.source += fields[1];
  prev.line += fields[2];
  prev.source_column += fields[3];
  if (prev.source < 0 || static_cast<std::size_t>(prev.source) >= sources_.size()) {
    throw Error("source map: segment names source #" + std::to_string(prev.source) + " but only " +
                std::to_string(sources_.size()) + " are listed");
  }
  if (lines_.size() <= line) lines_.resize(line + 1);
  lines_[line].push_back(prev);
}

std::optional<SourcePos> SourceMap::lookup(int line, int column) const {
  if (line < 1 || static_cast<std::size_t>(line) > lines_.size()) return std::nullopt;
  const auto& segments = lines_[static_cast<std::size_t>(line - 1)];
  const auto after = std::upper_bound(segments.begin(), segments.end(), column - 1,
                                      [](int col, const Segment& s) { return col < s.column; });
  if (segments.empty() || (after == segments.begin() && column > 0)) return std::nullopt;
  const Segment& s = after == segments.begin() ? segments.front() : *std::prev(after);
  return SourcePos{sources_[static_cast<std::size_t>(s.source)], s.line + 1, s.source_column + 1};
}

std::vector<int> decode_vlq(std::string_view text) {
  std::vector<int> values;
  int value = 0;
  int shift = 0;
  for (const char c : text) {
    const int digit = digit_of(c);
    value += (digit & 31) << shift;
    shift += 5;
    if (digit & 32) continue;
    values.push_back(value & 1 ? -(value >> 1) : value >> 1);
    value = 0;
    shift = 0;
  }
  if (shift != 0) throw Error("source map: VLQ value \"" + std::string(text) + "\" ends mid-value");
  return values;
}

std::string decode_base64(std::string_view text) {
  std::string out;
  unsigned bits = 0;
  int count = 0;
  for (const char c : text) {
    if (c == '=') break;
    bits = (bits << 6) | static_cast<unsigned>(digit_of(c));
    count += 6;
    if (count < 8) continue;
    count -= 8;
    out.push_back(static_cast<char>((bits >> count) & 0xFF));
  }
  return out;
}

std::optional<std::string> inline_source_map(std::string_view js) {
  const std::size_t at = js.rfind(kDataUrl);
  if (at == std::string_view::npos) return std::nullopt;
  const std::size_t start = at + kDataUrl.size();
  const std::size_t end = std::min(js.find_first_of("\r\n", start), js.size());
  return decode_base64(js.substr(start, end - start));
}

std::string rewrite_locations(std::string_view text, std::string_view file, const SourceMap& map) {
  std::string out;
  std::size_t copied = 0;
  for (std::size_t at = text.find(file); at != std::string_view::npos; at = text.find(file, at + 1)) {
    const auto loc = location_at(text, at + file.size());
    if (!loc) continue;
    const auto pos = map.lookup(loc->line, loc->column);
    if (!pos) continue;
    out.append(text.substr(copied, at - copied));
    out += pos->file + ":" + std::to_string(pos->line) + ":" + std::to_string(pos->column);
    copied = loc->end;
  }
  out.append(text.substr(copied));
  return out;
}

}  // namespace alpha::core
