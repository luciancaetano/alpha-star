#include "core/bytecode.hpp"

#include <monocypher.h>

#include <algorithm>

#include "core/error.hpp"

namespace alpha::core {

namespace {

constexpr std::string_view kMagic = "ALPHAJBC";
constexpr std::string_view kName = kBytecodeFile;

struct Slot {
  std::size_t at;
  std::size_t size;
};
constexpr Slot kFormat{8, 4}, kLength{12, 4}, kKeyId{16, 16}, kNonce{32, 24}, kTag{56, 16}, kEngine{72, 12},
    kQuickjs{84, 12}, kBuild{96, 16};

void put_number(std::string& out, Slot s, std::uint64_t value) {
  for (std::size_t i = 0; i < s.size; ++i) out[s.at + i] = static_cast<char>((value >> (8 * i)) & 0xFF);
}

std::uint64_t number(std::string_view in, Slot s) {
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < s.size; ++i) value |= std::uint64_t{static_cast<unsigned char>(in[s.at + i])} << (8 * i);
  return value;
}

void put_text(std::string& out, Slot s, std::string_view value) {
  value.substr(0, s.size).copy(out.data() + s.at, s.size);
}

std::uint8_t* bytes_at(std::string& s, std::size_t at) { return reinterpret_cast<std::uint8_t*>(s.data() + at); }

const std::uint8_t* bytes_at(std::string_view s, std::size_t at) {
  return reinterpret_cast<const std::uint8_t*>(s.data() + at);
}

// The slot without its NUL padding; any other byte outside printable ASCII
// (a NUL inside included) reads as '?', so every changed byte changes the text.
std::string text(std::string_view in, Slot s) {
  std::string_view field = in.substr(s.at, s.size);
  field = field.substr(0, field.find_last_not_of('\0') + 1);
  std::string out;
  for (const char c : field) out += c >= ' ' && c <= '~' ? c : '?';
  return out;
}

// The associated data: the header with its tag slot zeroed.
std::string associated(std::string_view file) {
  std::string ad(file.substr(0, kBytecodeHeader));
  ad.replace(kTag.at, kTag.size, kTag.size, '\0');
  return ad;
}

// Size, magic and format: is this a game.jsbc this engine can read at all?
void check_frame(std::string_view file) {
  if (file.size() < 16 || file.substr(0, kMagic.size()) != kMagic) {
    throw Error(std::string(kName) + " is not alpha-star bytecode (" + std::to_string(file.size()) +
                " bytes, no ALPHAJBC header): re-export the game");
  }
  if (const auto format = number(file, kFormat); format != kBytecodeFormat) {
    const std::string old = format == 1 ? " (unencrypted, from an older SDK)" : "";
    throw Error(std::string(kName) + " uses bytecode format " + std::to_string(format) + old +
                "; this engine reads format " + std::to_string(kBytecodeFormat) +
                ": re-export the game with the SDK that matches this engine");
  }
  if (file.size() < kBytecodeHeader || number(file, kLength) != file.size() - kBytecodeHeader) {
    throw Error(std::string(kName) + " is truncated or damaged (" + std::to_string(file.size()) +
                " bytes, header says body " + std::to_string(number(file, kLength)) +
                "): re-export or reinstall the game");
  }
}

void check_origin(std::string_view file, const BuildStamp& self, const BytecodeKey& key) {
  if (const BuildStamp made = stamp_of(file); !(made == self)) {
    throw Error(std::string(kName) + " was compiled for " + describe(made) + ", but this engine is " +
                describe(self) + ": update the game (re-export it with the SDK that matches this engine)");
  }
  if (const std::string id = text(file, kKeyId); id != key.id) {
    throw Error(std::string(kName) + " was encrypted for engine key " + id + ", but this engine's key is " +
                key.id + ": re-export with this engine (compile and ship with engines from one staging)");
  }
}

}  // namespace

BytecodeKey::~BytecodeKey() { crypto_wipe(bytes.data(), bytes.size()); }

WipedBytes::~WipedBytes() { crypto_wipe(bytes.data(), bytes.size()); }

std::string seal_bytecode(std::string_view body, const BuildStamp& stamp, const BytecodeKey& key,
                          const BytecodeNonce& nonce) {
  std::string out(kBytecodeHeader + body.size(), '\0');
  put_text(out, {0, kMagic.size()}, kMagic);
  put_number(out, kFormat, kBytecodeFormat);
  put_number(out, kLength, body.size());
  put_text(out, kKeyId, key.id);
  std::copy(nonce.begin(), nonce.end(), bytes_at(out, kNonce.at));
  put_text(out, kEngine, stamp.engine);
  put_text(out, kQuickjs, stamp.quickjs);
  put_text(out, kBuild, stamp.build);
  const std::string ad = associated(out);
  crypto_aead_lock(bytes_at(out, kBytecodeHeader), bytes_at(out, kTag.at), key.bytes.data(), nonce.data(),
                   bytes_at(std::string_view(ad), 0), ad.size(), bytes_at(body, 0), body.size());
  return out;
}

BuildStamp stamp_of(std::string_view file) { return {text(file, kEngine), text(file, kQuickjs), text(file, kBuild)}; }

std::string describe(const BuildStamp& stamp) {
  return "alpha-star " + stamp.engine + " (QuickJS " + stamp.quickjs + ", build " + stamp.build + ")";
}

WipedBytes open_bytecode(std::string_view file, const BuildStamp& self, const BytecodeKey& key) {
  check_frame(file);
  check_origin(file, self, key);
  const std::string ad = associated(file);
  WipedBytes plain;
  plain.bytes.assign(file.size() - kBytecodeHeader, '\0');
  if (crypto_aead_unlock(bytes_at(plain.bytes, 0), bytes_at(file, kTag.at), key.bytes.data(),
                         bytes_at(file, kNonce.at), bytes_at(std::string_view(ad), 0), ad.size(),
                         bytes_at(file, kBytecodeHeader), plain.bytes.size()) != 0) {
    throw Error(std::string(kName) +
                " failed authentication; re-export with this engine (the file was changed or damaged)");
  }
  return plain;
}

}  // namespace alpha::core
