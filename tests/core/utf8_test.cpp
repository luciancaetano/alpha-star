#include <doctest/doctest.h>

#include "core/utf8.hpp"

using alpha::core::decode_utf8;
using alpha::core::utf8_fault;

TEST_CASE("UTF-8: every script decodes to its code points") {
  CHECK(decode_utf8("a\xc3\xa7\xd8\xb3\xe4\xbd\xa0\xf0\x9f\x98\x80") ==
        std::u32string{U'a', 0xE7, 0x633, 0x4F60, 0x1F600});
  CHECK_FALSE(utf8_fault("\xd0\x9f\xd1\x80\xd0\xb8 \xd7\xa9\xd7\x9c \xe0\xa4\xa8\xe0\xa4\xae \xe4\xbd\xa0"));
  std::string out;
  for (const char32_t c : {U'a', char32_t{0xE7}, char32_t{0x4F60}, char32_t{0x1F600}}) alpha::core::append_utf8(out, c);
  CHECK(out == "a\xc3\xa7\xe4\xbd\xa0\xf0\x9f\x98\x80");
}

TEST_CASE("UTF-8 faults name the byte and the reason; decoding replaces them") {
  const auto surrogate = utf8_fault("ok\xed\xa0\xbd!");  // an unpaired JS surrogate as WTF-8
  REQUIRE(surrogate);
  CHECK(surrogate->offset == 2);
  CHECK(surrogate->why.find("surrogate") != std::string::npos);
  CHECK(utf8_fault("\xc0\xaf")->why.find("cannot start") != std::string::npos);  // overlong 2-byte lead
  CHECK(utf8_fault("\xe0\x80\xaf")->why.find("overlong") != std::string::npos);
  CHECK(utf8_fault("\xf4\x90\x80\x80")->why.find("U+10FFFF") != std::string::npos);
  CHECK(utf8_fault("ab\xe4\xbd")->offset == 2);  // cut short at the end
  CHECK(decode_utf8("a\xffz") == std::u32string{U'a', 0xFFFD, U'z'});
}
