#include <doctest/doctest.h>

#include <string>
#include <string_view>

#include "core/bytecode.hpp"
#include "core/error.hpp"

using alpha::core::BuildStamp;
using alpha::core::kBytecodeHeader;
using alpha::core::open_bytecode;
using alpha::core::seal_bytecode;
using alpha::core::stamp_of;

namespace {

const BuildStamp kSelf{"0.2.0", "0.10.1", "abcdef123456-64"};

// The message open_bytecode throws for `file`, or "" when it accepts it.
std::string rejection(std::string_view file, const BuildStamp& self = kSelf) {
  try {
    open_bytecode(file, self);
    return "";
  } catch (const alpha::core::Error& e) {
    return e.what();
  }
}

}  // namespace

TEST_CASE("bytecode: a sealed body opens again on the same build") {
  const std::string body("\x01\x02 module bytes \x00\xff", 18);
  const std::string file = seal_bytecode(body, kSelf);
  CHECK(file.size() == kBytecodeHeader + body.size());
  CHECK(file.substr(0, 8) == "ALPHAJBC");
  CHECK(open_bytecode(file, kSelf) == body);
  CHECK(stamp_of(file) == kSelf);
}

TEST_CASE("bytecode: another engine version, QuickJS or build is refused naming both") {
  const std::string file = seal_bytecode("body", {"0.1.0", "0.10.1", "abcdef123456-64"});
  const std::string why = rejection(file);
  CHECK(why.find("game.jsbc was compiled for alpha-star 0.1.0 (QuickJS 0.10.1, build abcdef123456-64)") !=
        std::string::npos);
  CHECK(why.find("this engine is alpha-star 0.2.0") != std::string::npos);
  CHECK(why.find("update") != std::string::npos);
  CHECK(rejection(seal_bytecode("body", {"0.2.0", "0.9.0", kSelf.build})).find("QuickJS 0.9.0") != std::string::npos);
  CHECK(rejection(seal_bytecode("body", {"0.2.0", "0.10.1", "ffff-32"})).find("build ffff-32") != std::string::npos);
}

TEST_CASE("bytecode: truncated, damaged and foreign files are refused with readable reasons") {
  const std::string file = seal_bytecode("some module body", kSelf);
  CHECK(rejection("").find("not alpha-star bytecode") != std::string::npos);
  CHECK(rejection(file.substr(0, 20)).find("not alpha-star bytecode") != std::string::npos);
  CHECK(rejection("export default 1;" + std::string(64, ' ')).find("not alpha-star bytecode") != std::string::npos);
  CHECK(rejection(file.substr(0, file.size() - 1)).find("truncated or damaged") != std::string::npos);
  CHECK(rejection(file + "x").find("truncated or damaged") != std::string::npos);
  std::string flipped = file;
  flipped[kBytecodeHeader + 3] ^= 0x20;
  CHECK(rejection(flipped).find("checksum") != std::string::npos);
  std::string newer = file;
  newer[8] = 9;
  CHECK(rejection(newer).find("format 9") != std::string::npos);
}

TEST_CASE("bytecode: every truncation and every single-byte change is refused") {
  const std::string file = seal_bytecode(std::string(200, 'q') + "tail", kSelf);
  for (std::size_t n = 0; n < file.size(); ++n) CHECK_FALSE(rejection(file.substr(0, n)).empty());
  for (std::size_t i = 0; i < file.size(); ++i) {
    std::string bad = file;
    bad[i] = static_cast<char>(bad[i] ^ 0x5a);
    CHECK_FALSE(rejection(bad).empty());
  }
}

TEST_CASE("bytecode: garbage in a stamp field prints as '?', never raw bytes") {
  std::string file = seal_bytecode("b", kSelf);
  file[24] = '\x07';
  const std::string why = rejection(file);
  CHECK(why.find("alpha-star ?.2.0") != std::string::npos);
}
