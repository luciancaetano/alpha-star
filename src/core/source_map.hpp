#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace alpha::core {

// A position in an original source file. Line and column are 1-based.
struct SourcePos {
  std::string file;
  int line = 0;
  int column = 0;
  bool operator==(const SourcePos&) const = default;
};

// Source map v3 ("mappings" + "sources"), as esbuild writes it, so errors in a
// bundle can name the TypeScript the agent wrote. Pure: no file or JSON access.
class SourceMap {
 public:
  // Throws Error if `mappings` is malformed or names a source that is not listed.
  // Leading "../" and "./" are dropped from source names (they are relative to the bundle).
  SourceMap(std::string_view mappings, std::vector<std::string> sources);
  // Generated 1-based line/column -> original position of the nearest segment
  // at or before it on that line (column <= 0: the line's first segment);
  // nullopt if nothing is mapped there.
  std::optional<SourcePos> lookup(int line, int column) const;

 private:
  struct Segment {
    int column;
    int source;
    int line;
    int source_column;
  };
  void add_segment(std::size_t line, const std::vector<int>& fields, Segment& prev);

  std::vector<std::vector<Segment>> lines_;
  std::vector<std::string> sources_;
};

// Base64 VLQ values of one segment ("2HwBG" -> 123, 24, 3). Throws Error if malformed.
std::vector<int> decode_vlq(std::string_view text);
// Standard base64 (padding optional). Throws Error on a character outside the alphabet.
std::string decode_base64(std::string_view text);
// JSON text of the last `//# sourceMappingURL=data:application/json;base64,...`, if any.
std::optional<std::string> inline_source_map(std::string_view js);
// Rewrites every "<file>:LINE:COL" (or "<file>:LINE") in `text` to the mapped
// original position; locations that are not mapped are kept as they are.
std::string rewrite_locations(std::string_view text, std::string_view file, const SourceMap& map);

}  // namespace alpha::core
