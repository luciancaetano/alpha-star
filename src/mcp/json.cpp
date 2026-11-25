#include "mcp/json.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "core/error.hpp"
#include "core/index_of.hpp"

namespace alpha::mcp::json {

namespace {

// Reads one flat object, left to right; `at` is the offset of the next byte.
class Reader {
 public:
  explicit Reader(std::string_view text) : text_(text) {}

  Object object() {
    expect('{');
    Object out;
    if (!take('}')) members(out);
    skip_space();
    if (at_ != text_.size()) fail("unexpected text after the object");
    return out;
  }

 private:
  void members(Object& out) {
    do {
      std::string key = string();
      expect(':');
      out[std::move(key)] = value();
    } while (take(','));
    expect('}');
  }

  Field value() {
    const char c = peek();
    if (c == '"') return string();
    if (c == '{' || c == '[') fail("nested objects and arrays are not supported; send flat fields");
    if (c == 't' || c == 'f' || c == 'n') return literal();
    return number();
  }

  Field literal() {
    for (const auto& [word, field] : {std::pair<std::string_view, Field>{"true", true},
                                      {"false", false}, {"null", nullptr}}) {
      if (text_.substr(at_, word.size()) == word) {
        at_ += word.size();
        return field;
      }
    }
    fail("expected true, false or null");
  }

  Field number() {
    double v = 0.0;
    const char* begin = text_.data() + at_;
    const auto [end, ec] = std::from_chars(begin, text_.data() + text_.size(), v);
    if (ec != std::errc() || end == begin) fail("expected a value");
    at_ += static_cast<std::size_t>(end - begin);
    return v;
  }

  std::string string() {
    expect('"');
    std::string out;
    for (char c = next(); c != '"'; c = next()) {
      if (c == '\\') escape(out);
      else if (static_cast<unsigned char>(c) < 0x20) fail("control character in a string");
      else out += c;
    }
    return out;
  }

  void escape(std::string& out) {
    const char c = next();
    static constexpr std::string_view kFrom = "\"\\/bfnrt";
    static constexpr std::string_view kTo = "\"\\/\b\f\n\r\t";
    if (const auto i = kFrom.find(c); i != std::string_view::npos) out += kTo[i];
    else if (c == 'u') utf8(out, code_point());
    else fail("unknown escape \\" + std::string(1, c));
  }

  // \uXXXX, combining a surrogate pair into one code point.
  std::uint32_t code_point() {
    const std::uint32_t high = hex4();
    if (high < 0xD800 || high > 0xDBFF) return high;
    if (next() != '\\' || next() != 'u') fail("unpaired surrogate");
    return 0x10000 + ((high - 0xD800) << 10) + (hex4() - 0xDC00);
  }

  std::uint32_t hex4() {
    std::uint32_t v = 0;
    const char* begin = text_.data() + at_;
    const auto [end, ec] = std::from_chars(begin, begin + std::min<std::size_t>(4, text_.size() - at_), v, 16);
    if (ec != std::errc() || end != begin + 4) fail("expected 4 hex digits after \\u");
    at_ += 4;
    return v;
  }

  static void utf8(std::string& out, std::uint32_t cp) {
    const auto byte = [&out](std::uint32_t b) { out += static_cast<char>(b); };
    if (cp < 0x80) return byte(cp);
    if (cp < 0x800) return byte(0xC0 | (cp >> 6)), byte(0x80 | (cp & 0x3F));
    if (cp < 0x10000) return byte(0xE0 | (cp >> 12)), byte(0x80 | ((cp >> 6) & 0x3F)), byte(0x80 | (cp & 0x3F));
    byte(0xF0 | (cp >> 18));
    byte(0x80 | ((cp >> 12) & 0x3F));
    byte(0x80 | ((cp >> 6) & 0x3F));
    byte(0x80 | (cp & 0x3F));
  }

  void skip_space() {
    while (at_ < text_.size() && std::string_view(" \t\r\n").find(text_[at_]) != std::string_view::npos) ++at_;
  }
  char peek() {
    skip_space();
    if (at_ >= text_.size()) fail("unexpected end of input");
    return text_[at_];
  }
  char next() {
    if (at_ >= text_.size()) fail("unexpected end of input");
    return text_[at_++];
  }
  bool take(char c) {
    if (peek() != c) return false;
    ++at_;
    return true;
  }
  void expect(char c) {
    if (!take(c)) fail(std::string("expected '") + c + "'");
  }
  [[noreturn]] void fail(const std::string& what) const {
    throw core::Error("bad JSON request at offset " + std::to_string(at_) + ": " + what);
  }

  std::string_view text_;
  std::size_t at_ = 0;
};

}  // namespace

Object parse_object(std::string_view text) { return Reader(text).object(); }

std::string str(std::string_view s) {
  std::string out = "\"";
  for (const char c : s) {
    if (c == '"' || c == '\\') out += {'\\', c};
    else if (c == '\n') out += "\\n";
    else if (static_cast<unsigned char>(c) < 0x20) {
      char buf[8];
      std::snprintf(buf, sizeof buf, "\\u%04x", static_cast<unsigned>(c));
      out += buf;
    } else out += c;
  }
  return out + "\"";
}

std::string num(double v) {
  if (!std::isfinite(v)) return "null";
  char buf[32];
  const auto [end, ec] = std::to_chars(buf, buf + sizeof buf, v);
  return std::string(buf, end);
}

std::string boolean(bool v) { return v ? "true" : "false"; }

std::string object(const std::vector<std::pair<std::string, std::string>>& fields) {
  std::string out = "{";
  for (const auto& [key, value] : fields) out += (out.size() > 1 ? "," : "") + str(key) + ":" + value;
  return out + "}";
}

std::string array(const std::vector<std::string>& items) { return "[" + core::join(items, ",") + "]"; }

std::string raw(const Field& f) {
  if (const auto* s = std::get_if<std::string>(&f)) return str(*s);
  if (const auto* d = std::get_if<double>(&f)) return num(*d);
  if (const auto* b = std::get_if<bool>(&f)) return boolean(*b);
  return "null";
}

}  // namespace alpha::mcp::json
