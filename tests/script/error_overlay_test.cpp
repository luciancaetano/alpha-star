#include <doctest/doctest.h>

#include "script/error_overlay.hpp"

using alpha::script::error_overlay;
using alpha::script::wrap;

TEST_CASE("wrap breaks at newlines, then spaces, then hard at the column") {
  CHECK(wrap("ab\ncd", 10) == std::vector<std::string>{"ab", "cd"});
  CHECK(wrap("one two three", 7) == std::vector<std::string>{"one two", "three"});
  CHECK(wrap("abcdefghij", 4) == std::vector<std::string>{"abcd", "efgh", "ij"});
}

TEST_CASE("error overlay stacks red lines from the top-left") {
  const auto items = error_overlay("x", 288);
  REQUIRE(items.size() == 2);
  CHECK(items[0].text == "SCRIPT ERROR (game paused)");
  CHECK(items[1].text == "x");
  CHECK(items[1].pos.y > items[0].pos.y);
  CHECK(items[0].color.r == 255);
}
