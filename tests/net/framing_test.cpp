#include <doctest/doctest.h>

#include "core/error.hpp"
#include "net/framing.hpp"

using namespace alpha::net;

TEST_CASE("framing names parse; unknown ones do not") {
  CHECK(parse_framing("lines") == Framing::lines);
  CHECK(parse_framing("u16") == Framing::u16);
  CHECK(parse_framing("u32") == Framing::u32);
  CHECK(parse_framing("raw") == Framing::raw);
  CHECK_FALSE(parse_framing("json"));
}

TEST_CASE("u32 frames round-trip even when bytes arrive one at a time") {
  const std::string wire = frame(Framing::u32, "hello") + frame(Framing::u32, "") + frame(Framing::u32, "ab");
  CHECK(wire.substr(0, 4) == std::string("\0\0\0\5", 4));
  Deframer d(Framing::u32, 100);
  std::deque<Message> out;
  for (char c : wire) CHECK(d.push(std::string_view(&c, 1), out).empty());
  REQUIRE(out.size() == 3);
  CHECK(out[0].data == "hello");
  CHECK(out[1].data.empty());
  CHECK(out[2].data == "ab");
  CHECK_FALSE(out[0].text);
}

TEST_CASE("u16 refuses payloads over 65535 and parses big-endian lengths") {
  CHECK_THROWS_AS(frame(Framing::u16, std::string(70000, 'x')), alpha::core::Error);
  Deframer d(Framing::u16, 1000);
  std::deque<Message> out;
  CHECK(d.push(std::string("\x01\x00", 2) + std::string(256, 'y'), out).empty());
  REQUIRE(out.size() == 1);
  CHECK(out[0].data.size() == 256);
}

TEST_CASE("lines strip \\n and \\r\\n, keep partial lines and are text") {
  Deframer d(Framing::lines, 100);
  std::deque<Message> out;
  CHECK(d.push("one\r\ntw", out).empty());
  CHECK(d.push("o\n", out).empty());
  REQUIRE(out.size() == 2);
  CHECK(out[0].data == "one");
  CHECK(out[1].data == "two");
  CHECK(out[0].text);
  CHECK(frame(Framing::lines, "hi") == "hi\n");
  CHECK_THROWS_AS(frame(Framing::lines, "a\nb"), alpha::core::Error);
}

TEST_CASE("a message over maxMessageBytes is an error naming both sizes") {
  Deframer d(Framing::u32, 1 << 20);
  std::deque<Message> out;
  CHECK(d.push(std::string("\x00\x20\x00\x00", 4), out) == "peer sent 2 MiB message, over maxMessageBytes 1 MiB");
  Deframer lines(Framing::lines, 4);
  CHECK_FALSE(lines.push("abcdefgh", out).empty());
}

TEST_CASE("raw delivers each chunk as one message") {
  Deframer d(Framing::raw, 100);
  std::deque<Message> out;
  CHECK(d.push("abc", out).empty());
  CHECK(d.push("", out).empty());
  REQUIRE(out.size() == 1);
  CHECK(out[0].data == "abc");
}

TEST_CASE("sizes read naturally in messages") {
  CHECK(size_text(1 << 20) == "1 MiB");
  CHECK(size_text(8192) == "8 KiB");
  CHECK(size_text(70000) == "70000 bytes");
  CHECK(std::string(state_name(State::closing)) == "closing");
  CHECK(std::string(kind_name(Kind::ws)) == "ws");
}
