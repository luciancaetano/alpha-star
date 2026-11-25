#include "core/bytecode.hpp"

#include "core/error.hpp"

namespace alpha::core {

namespace {

constexpr std::string_view kMagic = "ALPHAJBC";
constexpr std::string_view kName = kBytecodeFile;

struct Slot {
  std::size_t at;
  std::size_t size;
};
constexpr Slot kFormat{8, 4}, kLength{12, 4}, kSum{16, 8}, kEngine{24, 12}, kQuickjs{36, 12}, kBuild{48, 16};

std::uint64_t fnv1a(std::string_view bytes) {
  std::uint64_t hash = 0xcbf29ce484222325ULL;
  for (const char c : bytes) hash = (hash ^ static_cast<unsigned char>(c)) * 0x100000001b3ULL;
  return hash;
}

void put_number(std::string& out, Slot s, std::uint64_t value) {
  for (std::size_t i = 0; i < s.size; ++i) out[s.at + i] = static_cast<char>((value >> (8 * i)) & 0xFF);
}

std::uint64_t number(std::string_view in, Slot s) {
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < s.size; ++i) value |= std::uint64_t{static_cast<unsigned char>(in[s.at + i])} << (8 * i);
  return value;
}

void put_text(std::string& out, Slot s, std::string_view value) { value.copy(out.data() + s.at, s.size); }

// The slot without its NUL padding; any other byte outside printable ASCII
// (a NUL inside included) reads as '?', so every changed byte changes the text.
std::string text(std::string_view in, Slot s) {
  std::string_view field = in.substr(s.at, s.size);
  field = field.substr(0, field.find_last_not_of('\0') + 1);
  std::string out;
  for (const char c : field) out += c >= ' ' && c <= '~' ? c : '?';
  return out;
}

// Size, magic and format: is this a game.jsbc this engine can read at all?
void check_frame(std::string_view file) {
  if (file.size() < kBytecodeHeader || file.substr(0, kMagic.size()) != kMagic) {
    throw Error(std::string(kName) + " is not alpha-star bytecode (" + std::to_string(file.size()) +
                " bytes, no ALPHAJBC header): re-export the game");
  }
  if (const auto format = number(file, kFormat); format != kBytecodeFormat) {
    throw Error(std::string(kName) + " uses bytecode format " + std::to_string(format) + "; this engine reads format " +
                std::to_string(kBytecodeFormat) + ": update the game or the engine so they match");
  }
}

void check_body(std::string_view file, std::string_view body) {
  if (number(file, kLength) != body.size()) {
    throw Error(std::string(kName) + " is truncated or damaged (body " + std::to_string(body.size()) +
                " bytes, header says " + std::to_string(number(file, kLength)) + "): re-export or reinstall the game");
  }
  if (number(file, kSum) != fnv1a(body)) {
    throw Error(std::string(kName) + " is damaged (checksum mismatch): re-export or reinstall the game");
  }
}

}  // namespace

std::string seal_bytecode(std::string_view body, const BuildStamp& stamp) {
  std::string out(kBytecodeHeader, '\0');
  put_text(out, {0, kMagic.size()}, kMagic);
  put_number(out, kFormat, kBytecodeFormat);
  put_number(out, kLength, body.size());
  put_number(out, kSum, fnv1a(body));
  put_text(out, kEngine, stamp.engine);
  put_text(out, kQuickjs, stamp.quickjs);
  put_text(out, kBuild, stamp.build);
  return out.append(body);
}

BuildStamp stamp_of(std::string_view file) { return {text(file, kEngine), text(file, kQuickjs), text(file, kBuild)}; }

std::string describe(const BuildStamp& stamp) {
  return "alpha-star " + stamp.engine + " (QuickJS " + stamp.quickjs + ", build " + stamp.build + ")";
}

std::string_view open_bytecode(std::string_view file, const BuildStamp& self) {
  check_frame(file);
  if (const BuildStamp made = stamp_of(file); !(made == self)) {
    throw Error(std::string(kName) + " was compiled for " + describe(made) + ", but this engine is " + describe(self) +
                ": update the game (re-export it with the SDK that matches this engine)");
  }
  const std::string_view body = file.substr(kBytecodeHeader);
  check_body(file, body);
  return body;
}

}  // namespace alpha::core
