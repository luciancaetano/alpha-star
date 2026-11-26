#include <doctest/doctest.h>

#include <string>
#include <string_view>

#include "core/bytecode.hpp"
#include "core/error.hpp"

using alpha::core::BuildStamp;
using alpha::core::BytecodeKey;
using alpha::core::BytecodeNonce;
using alpha::core::kBytecodeHeader;
using alpha::core::open_bytecode;
using alpha::core::seal_bytecode;
using alpha::core::stamp_of;

namespace {

const BuildStamp kSelf{"0.2.0", "0.10.1", "abcdef123456-64"};

void fill_key(BytecodeKey& key, std::uint8_t seed, const char* id) {
  for (std::size_t i = 0; i < key.bytes.size(); ++i) key.bytes[i] = static_cast<std::uint8_t>(seed + i * 7);
  key.id = id;
}

struct Keys {
  BytecodeKey self, other, same_id;
  Keys() {
    fill_key(self, 1, "00112233aabbccdd");
    fill_key(other, 9, "ffeeddcc99887766");
    fill_key(same_id, 9, "00112233aabbccdd");  // another key under this engine's id
  }
};
const Keys kKeys;

BytecodeNonce nonce(std::uint8_t seed) {
  BytecodeNonce n{};
  for (std::size_t i = 0; i < n.size(); ++i) n[i] = static_cast<std::uint8_t>(seed * 31 + i);
  return n;
}

std::string seal(std::string_view body, const BuildStamp& stamp = kSelf, const BytecodeKey& key = kKeys.self) {
  return seal_bytecode(body, stamp, key, nonce(3));
}

// The message open_bytecode throws for `file`, or "" when it accepts it.
std::string rejection(std::string_view file, const BuildStamp& self = kSelf) {
  try {
    open_bytecode(file, self, kKeys.self);
    return "";
  } catch (const alpha::core::Error& e) {
    return e.what();
  }
}

}  // namespace

TEST_CASE("bytecode: a sealed body is encrypted and opens again on the same build and key") {
  const std::string body("\x01\x02 module bytes PLAIN_MARKER \x00\xff", 31);
  const std::string file = seal(body);
  CHECK(file.size() == kBytecodeHeader + body.size());
  CHECK(file.substr(0, 8) == "ALPHAJBC");
  CHECK(file.find("PLAIN_MARKER") == std::string::npos);
  CHECK(open_bytecode(file, kSelf, kKeys.self).bytes == body);
  CHECK(stamp_of(file) == kSelf);
}

TEST_CASE("bytecode: another nonce gives another file that opens to the same body") {
  const std::string a = seal_bytecode("same body", kSelf, kKeys.self, nonce(1));
  const std::string b = seal_bytecode("same body", kSelf, kKeys.self, nonce(2));
  CHECK(a != b);
  CHECK(a.substr(kBytecodeHeader) != b.substr(kBytecodeHeader));
  CHECK(open_bytecode(b, kSelf, kKeys.self).bytes == "same body");
}

TEST_CASE("bytecode: another engine version, QuickJS or build is refused naming both") {
  const std::string why = rejection(seal("body", {"0.1.0", "0.10.1", "abcdef123456-64"}));
  CHECK(why.find("game.jsbc was compiled for alpha-star 0.1.0 (QuickJS 0.10.1, build abcdef123456-64)") !=
        std::string::npos);
  CHECK(why.find("this engine is alpha-star 0.2.0") != std::string::npos);
  CHECK(rejection(seal("body", {"0.2.0", "0.9.0", kSelf.build})).find("QuickJS 0.9.0") != std::string::npos);
  CHECK(rejection(seal("body", {"0.2.0", "0.10.1", "ffff-32"})).find("build ffff-32") != std::string::npos);
}

TEST_CASE("bytecode: a file for another key is refused naming both keys") {
  const std::string why = rejection(seal("body", kSelf, kKeys.other));
  CHECK(why.find("encrypted for engine key ffeeddcc99887766") != std::string::npos);
  CHECK(why.find("this engine's key is 00112233aabbccdd") != std::string::npos);
  CHECK(rejection(seal("body", kSelf, kKeys.same_id)).find("failed authentication; re-export with this engine") !=
        std::string::npos);
}

TEST_CASE("bytecode: truncated, tampered, old and foreign files are refused with readable reasons") {
  const std::string file = seal("some module body");
  CHECK(rejection("").find("not alpha-star bytecode") != std::string::npos);
  CHECK(rejection("export default 1;" + std::string(120, ' ')).find("not alpha-star bytecode") != std::string::npos);
  CHECK(rejection(file.substr(0, 20)).find("truncated or damaged") != std::string::npos);
  CHECK(rejection(file.substr(0, file.size() - 1)).find("truncated or damaged") != std::string::npos);
  CHECK(rejection(file + "x").find("truncated or damaged") != std::string::npos);
  std::string flipped = file;
  flipped[kBytecodeHeader + 3] ^= 0x20;
  CHECK(rejection(flipped).find("game.jsbc failed authentication; re-export with this engine") != std::string::npos);
  std::string old = file;
  old[8] = 1;
  CHECK(rejection(old).find("format 1 (unencrypted") != std::string::npos);
  old[8] = 9;
  CHECK(rejection(old).find("format 9") != std::string::npos);
}

TEST_CASE("bytecode: every truncation and every single-byte change is refused") {
  const std::string file = seal(std::string(200, 'q') + "tail");
  for (std::size_t n = 0; n < file.size(); ++n) CHECK_FALSE(rejection(file.substr(0, n)).empty());
  for (std::size_t i = 0; i < file.size(); ++i) {
    std::string bad = file;
    bad[i] = static_cast<char>(bad[i] ^ 0x5a);
    CHECK_FALSE(rejection(bad).empty());
  }
}

TEST_CASE("bytecode: garbage in a stamp field prints as '?', never raw bytes") {
  std::string file = seal("b");
  file[72] = '\x07';
  CHECK(rejection(file).find("alpha-star ?.2.0") != std::string::npos);
}
