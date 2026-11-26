#include "core/bmfont.hpp"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <map>

#include "core/error.hpp"

namespace alpha::core {

namespace {

// One line of the text format or one element of the XML format:
// its tag and key=value attributes (values unquoted).
struct Record {
  std::string tag;
  std::map<std::string, std::string, std::less<>> attrs;
};

bool space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }

std::string_view word(std::string_view s, std::size_t& i) {
  while (i < s.size() && space(s[i])) ++i;
  const std::size_t start = i;
  while (i < s.size() && !space(s[i]) && s[i] != '=' && s[i] != '/' && s[i] != '>') ++i;
  return s.substr(start, i - start);
}

std::string value(std::string_view s, std::size_t& i) {
  if (i < s.size() && s[i] == '"') {
    const std::size_t end = s.find('"', i + 1);
    const std::size_t stop = end == std::string_view::npos ? s.size() : end;
    std::string out(s.substr(i + 1, stop - i - 1));
    i = stop + 1;
    return out;
  }
  const std::size_t start = i;
  while (i < s.size() && !space(s[i]) && s[i] != '/' && s[i] != '>') ++i;
  return std::string(s.substr(start, i - start));
}

Record record(std::string_view s) {
  std::size_t i = 0;
  Record r{std::string(word(s, i)), {}};
  for (std::string_view key = word(s, i); !key.empty(); key = word(s, i)) {
    if (i < s.size() && s[i] == '=') ++i;
    r.attrs[std::string(key)] = value(s, i);
  }
  return r;
}

// Text: one record per line. XML: one per element ("<char .../>").
std::vector<Record> records(std::string_view text) {
  const char split = text.find('<') != std::string_view::npos ? '<' : '\n';
  std::vector<Record> out;
  for (std::size_t start = 0; start < text.size();) {
    std::size_t end = text.find(split, start);
    if (end == std::string_view::npos) end = text.size();
    Record r = record(text.substr(start, end - start));
    if (!r.tag.empty()) out.push_back(std::move(r));
    start = end + 1;
  }
  return out;
}

float number(const Record& r, std::string_view key) {
  const auto it = r.attrs.find(key);
  if (it == r.attrs.end()) return 0.0f;
  int v = 0;
  const std::string& s = it->second;
  if (std::from_chars(s.data(), s.data() + s.size(), v).ec != std::errc{}) {
    throw Error("BMFont: " + r.tag + " " + std::string(key) + "=\"" + s + "\" is not a whole number");
  }
  return static_cast<float>(v);
}

int whole(const Record& r, std::string_view key) { return static_cast<int>(number(r, key)); }

BmChar char_of(const Record& r) {
  return {static_cast<char32_t>(whole(r, "id")),
          {whole(r, "x"), whole(r, "y"), whole(r, "width"), whole(r, "height")},
          {number(r, "xoffset"), number(r, "yoffset")},
          number(r, "xadvance"),
          whole(r, "page")};
}

void add(BmFont& font, const Record& r, bool& common) {
  if (r.tag == "info") font.size = std::abs(whole(r, "size"));
  if (r.tag == "common") {
    font.line_height = number(r, "lineHeight");
    font.base = number(r, "base");
    common = true;
  }
  if (r.tag == "page") {
    const auto id = static_cast<std::size_t>(std::max(0, whole(r, "id")));
    if (font.pages.size() <= id) font.pages.resize(id + 1);
    font.pages[id] = r.attrs.contains("file") ? r.attrs.find("file")->second : "";
  }
  if (r.tag == "char") font.chars.push_back(char_of(r));
  if (r.tag == "kerning") {
    font.kernings.push_back({static_cast<char32_t>(whole(r, "first")), static_cast<char32_t>(whole(r, "second")),
                             number(r, "amount")});
  }
}

void check(const BmFont& font, bool common) {
  if (!common) throw Error("BMFont: no \"common\" line (lineHeight, base); is this a .fnt file?");
  if (font.pages.empty()) throw Error("BMFont: no \"page\" line naming its image");
  for (const BmChar& c : font.chars) {
    if (c.page < 0 || static_cast<std::size_t>(c.page) >= font.pages.size()) {
      throw Error("BMFont: char id=" + std::to_string(static_cast<unsigned>(c.id)) + " is on page " + std::to_string(c.page) + ", the font has " +
                  std::to_string(font.pages.size()) + " page(s)");
    }
  }
}

}  // namespace

BmFont parse_bmfont(std::string_view text) {
  if (text.starts_with("BMF")) throw Error("BMFont: binary .fnt files are not supported; export it as text or XML");
  BmFont font;
  bool common = false;
  for (const Record& r : records(text)) add(font, r, common);
  check(font, common);
  if (font.size == 0) font.size = static_cast<int>(font.line_height);
  return font;
}

BitmapAtlas bitmap_atlas(const BmFont& font, const std::vector<int>& page_top) {
  BitmapAtlas out;
  out.atlas.line_height = font.line_height;
  out.atlas.ascent = font.base;
  for (const BmChar& c : font.chars) {
    Region src = c.src;
    src.y += page_top.at(static_cast<std::size_t>(c.page));
    out.atlas.glyphs[glyph_key(0, static_cast<std::uint32_t>(c.id))] = {src, c.offset, c.advance};
    if (c.id == U'x') out.atlas.x_height = static_cast<float>(c.src.h);
  }
  if (out.atlas.x_height <= 0.0f) out.atlas.x_height = font.base * 0.5f;
  for (const BmKerning& k : font.kernings) out.kerning[{k.first, k.second}] = k.amount;
  return out;
}

}  // namespace alpha::core
