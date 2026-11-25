#include <doctest/doctest.h>

#include <cmath>
#include <limits>
#include <string>

#include "core/error.hpp"
#include "mcp/base64.hpp"
#include "mcp/json.hpp"

namespace json = alpha::mcp::json;
using alpha::core::Error;

namespace {

std::string error_of(std::string_view text) {
  try {
    json::parse_object(text);
  } catch (const Error& e) {
    return e.what();
  }
  return "";
}

}  // namespace

TEST_CASE("json: a flat object with every value kind") {
  const auto o = json::parse_object(R"( {"cmd":"press", "n": -2.5e1, "on":true, "off" : false, "x":null} )");
  CHECK(std::get<std::string>(o.at("cmd")) == "press");
  CHECK(std::get<double>(o.at("n")) == -25.0);
  CHECK(std::get<bool>(o.at("on")));
  CHECK_FALSE(std::get<bool>(o.at("off")));
  CHECK(std::holds_alternative<std::nullptr_t>(o.at("x")));
  CHECK(json::parse_object("{}").empty());
}

TEST_CASE("json: string escapes, including unicode and surrogate pairs") {
  const auto o = json::parse_object(R"({"s":"a\"b\\c\/d\n\t\)" R"(u00e9\)" R"(ud83d\)" R"(ude00"})");
  CHECK(std::get<std::string>(o.at("s")) == "a\"b\\c/d\n\t\xc3\xa9\xf0\x9f\x98\x80");
}

TEST_CASE("json: malformed input is an error that says what and where") {
  CHECK(error_of(R"({"a":{"b":1}})").find("nested") != std::string::npos);
  CHECK(error_of(R"({"a":[1]})").find("nested") != std::string::npos);
  CHECK(error_of(R"({"a":1)").find("offset") != std::string::npos);
  CHECK(error_of(R"({"a" 1})") != "");
  CHECK(error_of(R"({"a":"x)") != "");
  CHECK(error_of(R"({"a":tru})") != "");
  CHECK(error_of(R"({"a":1} x)") != "");
  CHECK(error_of(R"([1])") != "");
  CHECK(error_of("") != "");
  CHECK(error_of(R"({"a":"\q"})") != "");
  CHECK(error_of(R"({"a":1,})") != "");
}

TEST_CASE("json: writers escape and compose") {
  CHECK(json::str("a\"b\\\n\x01") == R"("a\"b\\\n\u0001")");
  CHECK(json::num(3) == "3");
  CHECK(json::num(0.5) == "0.5");
  CHECK(json::num(std::numeric_limits<double>::quiet_NaN()) == "null");
  CHECK(json::boolean(true) == "true");
  CHECK(json::object({{"a", "1"}, {"b", json::str("x")}}) == R"({"a":1,"b":"x"})");
  CHECK(json::array({"1", "2"}) == "[1,2]");
  CHECK(json::array({}) == "[]");
  CHECK(json::raw(json::Field{std::string("id")}) == R"("id")");
  CHECK(json::raw(json::Field{7.0}) == "7");
  CHECK(json::raw(json::Field{nullptr}) == "null");
  CHECK(json::raw(json::Field{false}) == "false");
}

TEST_CASE("base64 encodes with padding") {
  CHECK(alpha::mcp::base64("") == "");
  CHECK(alpha::mcp::base64("f") == "Zg==");
  CHECK(alpha::mcp::base64("fo") == "Zm8=");
  CHECK(alpha::mcp::base64("foo") == "Zm9v");
  CHECK(alpha::mcp::base64(std::string("\x89PNG", 4)) == "iVBORw==");
}
