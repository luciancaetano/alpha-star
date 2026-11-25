#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "core/error.hpp"
#include "core/source_map.hpp"

using alpha::core::decode_base64;
using alpha::core::decode_vlq;
using alpha::core::inline_source_map;
using alpha::core::rewrite_locations;
using alpha::core::SourceMap;
using alpha::core::SourcePos;

TEST_CASE("decode_vlq reads signed base64 VLQ values") {
  CHECK(decode_vlq("AAAA") == std::vector<int>{0, 0, 0, 0});
  CHECK(decode_vlq("C") == std::vector<int>{1});
  CHECK(decode_vlq("D") == std::vector<int>{-1});
  CHECK(decode_vlq("gB") == std::vector<int>{16});      // continuation bit
  CHECK(decode_vlq("2HwBG") == std::vector<int>{123, 24, 3});
  CHECK_THROWS_AS(decode_vlq("A!"), alpha::core::Error);
  CHECK_THROWS_AS(decode_vlq("g"), alpha::core::Error);  // ends mid-value
}

TEST_CASE("decode_base64 decodes padded and unpadded text") {
  CHECK(decode_base64("aGVsbG8=") == "hello");
  CHECK(decode_base64("aGk") == "hi");
  CHECK(decode_base64("") == "");
  CHECK_THROWS_AS(decode_base64("a$b="), alpha::core::Error);
}

TEST_CASE("inline_source_map extracts the JSON of the last data-url comment") {
  const std::string js = "let a;\n//# sourceMappingURL=data:application/json;base64,eyJ2IjoxfQ==\n";
  CHECK(inline_source_map(js) == std::string("{\"v\":1}"));
  CHECK_FALSE(inline_source_map("let a;\n"));
}

namespace {
// Generated line 1: col 0 -> a.ts 1:0, col 4 -> a.ts 1:6. Line 2: empty.
// Line 3: col 2 -> b.ts 2:15 (1-based).
SourceMap sample() { return SourceMap("AAAA,IAAM;;ECCQ", {"../src/a.ts", "./src/b.ts"}); }
}  // namespace

TEST_CASE("lookup maps a generated position to the nearest segment at or before it") {
  const SourceMap map = sample();
  CHECK(map.lookup(1, 1) == SourcePos{"src/a.ts", 1, 1});
  CHECK(map.lookup(1, 3) == SourcePos{"src/a.ts", 1, 1});
  CHECK(map.lookup(1, 5) == SourcePos{"src/a.ts", 1, 7});
  CHECK(map.lookup(1, 99) == SourcePos{"src/a.ts", 1, 7});
  CHECK_FALSE(map.lookup(2, 1));
  CHECK(map.lookup(3, 3) == SourcePos{"src/b.ts", 2, 15});
  CHECK_FALSE(map.lookup(3, 1));   // before the first segment
  CHECK_FALSE(map.lookup(40, 1));  // past the last line
}

TEST_CASE("a mapping that names a missing source is rejected") {
  CHECK_THROWS_AS(SourceMap("ACAA", {"only.ts"}), alpha::core::Error);
}

TEST_CASE("rewrite_locations replaces every mapped file:line:col of the bundle") {
  const SourceMap map = sample();
  const std::string stack =
      "Error: boom\n    at update (C:/game/dist/main.js:1:5)\n    at <eval> (C:/game/dist/main.js:3:3)\n"
      "    at step (alpha:runtime:12:3)\n    at other (C:/game/dist/main.js:2:1)\n";
  CHECK(rewrite_locations(stack, "C:/game/dist/main.js", map) ==
        "Error: boom\n    at update (src/a.ts:1:7)\n    at <eval> (src/b.ts:2:15)\n"
        "    at step (alpha:runtime:12:3)\n    at other (C:/game/dist/main.js:2:1)\n");
}

TEST_CASE("rewrite_locations accepts file:line without a column") {
  CHECK(rewrite_locations("at f (main.js:3)", "main.js", sample()) == "at f (src/b.ts:2:15)");
}
