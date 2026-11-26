#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "core/bytecode.hpp"
#include "script/bytecode.hpp"
#include "script/host.hpp"
#include "host_fixture.hpp"

#if ALPHA_DEBUG_TOOLS  // --compile lives in dev/release engines

using alpha::core::BytecodeKey;
using alpha::core::BytecodeNonce;
using alpha::core::open_bytecode;
using alpha::core::seal_bytecode;
using alpha::script::compile_file;
using alpha::script::Host;
using alpha::script::this_build;
using alpha::script::this_key;
using alpha::test::Sandbox;
namespace fs = std::filesystem;

namespace {

constexpr float kDt = 1.0f / 60.0f;

std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), {}};
}

void write_file(const fs::path& path, const std::string& bytes) {
  fs::create_directories(path.parent_path());
  std::ofstream(path, std::ios::binary) << bytes;
}

// A game whose title proves which entry ran; its update counts ticks in the title text.
std::string game(const std::string& title) {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "// SECRET_COMMENT_TEXT must never reach the bytecode\n"
         "function secretHelperBody() { return 'helper' + 42; }\n"
         "const play = scene('play', { update(ctx) { secretHelperBody(); } });\n"
         "export default game({ config: { title: '" + title + "', size: [64, 32], seed: 1, actions: {} },\n"
         "  start: 'play', scenes: { play } });\n";
}

// Compiles `title`'s game into dir/pack/game.jsbc; returns the pack dir.
fs::path compiled_pack(const Sandbox& sb, const std::string& title) {
  const std::string source = sb.write("main.js", game(title));
  const fs::path pack = sb.dir / "pack";
  fs::create_directories(pack);
  REQUIRE(compile_file(source, (pack / "game.jsbc").string()) == 0);
  return pack;
}

// `body` sealed for this engine's key under `stamp`.
std::string sealed(const std::string& body, const alpha::core::BuildStamp& stamp) {
  BytecodeKey key;
  this_key(key);
  return seal_bytecode(body, stamp, key, BytecodeNonce{});
}

// The decrypted bytecode inside a game.jsbc of this engine.
std::string plain_of(const std::string& file) {
  BytecodeKey key;
  this_key(key);
  return open_bytecode(file, this_build(), key).bytes;
}

// The error a Host shows for a pack holding `bytes` as game.jsbc ("" when it boots).
std::string boot_error(const Sandbox& sb, const std::string& bytes) {
  const fs::path pack = sb.dir / "bad";
  write_file(pack / "game.jsbc", bytes);
  Host host(sb.save());
  host.load(pack.string());
  return host.error();
}

}  // namespace

TEST_CASE("bytecode: a compiled pack boots and steps without main.js") {
  Sandbox sb;
  const fs::path pack = compiled_pack(sb, "from bytecode");
  Host host(sb.save());
  CHECK(host.load(pack.string()).title == "from bytecode");
  for (int i = 0; i < 3; ++i) host.step({}, kDt);
  CHECK_MESSAGE(!host.paused(), host.error());
}

TEST_CASE("bytecode: game.jsbc wins over main.js in the same pack") {
  Sandbox sb;
  const fs::path pack = compiled_pack(sb, "jsbc");
  write_file(pack / "main.js", game("js"));
  Host host(sb.save());
  CHECK(host.load(pack.string()).title == "jsbc");
}

TEST_CASE("bytecode: no source text, comments or build paths in the bytecode") {
  Sandbox sb;
  const std::string bytes = plain_of(read_file(compiled_pack(sb, "t") / "game.jsbc"));
  CHECK(bytes.find("SECRET_COMMENT_TEXT") == std::string::npos);
  CHECK(bytes.find("return 'helper'") == std::string::npos);
  CHECK(bytes.find(sb.dir.filename().string()) == std::string::npos);
  CHECK(bytes.find("main.js") != std::string::npos);  // the module name only
  CHECK(bytes.find("secretHelperBody") != std::string::npos);  // names stay in the bytecode...
}

TEST_CASE("bytecode: the file is encrypted with a fresh nonce on every compile") {
  Sandbox sb;
  const std::string first = read_file(compiled_pack(sb, "t") / "game.jsbc");
  const std::string second = read_file(compiled_pack(sb, "t") / "game.jsbc");
  CHECK(first.find("secretHelperBody") == std::string::npos);  // ...but not in the file
  CHECK(first.find("main.js") == std::string::npos);
  CHECK(first.substr(32, 24) != second.substr(32, 24));
  CHECK(plain_of(first) == plain_of(second));
}

TEST_CASE("bytecode: wrong build, truncation and damage pause with readable errors") {
  Sandbox sb;
  const std::string good = read_file(compiled_pack(sb, "t") / "game.jsbc");
  const std::string body = plain_of(good);
  CHECK(boot_error(sb, good).empty());
  CHECK(boot_error(sb, sealed(body, {"9.9.9", this_build().quickjs, this_build().build}))
            .find("compiled for alpha-star 9.9.9") != std::string::npos);
  CHECK(boot_error(sb, good.substr(0, good.size() / 2)).find("truncated or damaged") != std::string::npos);
  CHECK(boot_error(sb, "export default 1;").find("not alpha-star bytecode") != std::string::npos);
  std::string flipped = good;
  flipped[flipped.size() - 5] ^= 0x11;
  CHECK(boot_error(sb, flipped).find("failed authentication") != std::string::npos);
}

TEST_CASE("bytecode: a well-stamped header over a foreign body is an error, never a crash") {
  Sandbox sb;
  CHECK_FALSE(boot_error(sb, sealed("", this_build())).empty());
  CHECK_FALSE(boot_error(sb, sealed("not bytecode at all", this_build())).empty());
}

// Not tested: a cut body re-encrypted with this engine's key (a deliberately crafted
// file, needs the key out of the engine) can crash QuickJS's reader. Authentication is
// the defense: every truncation, damage or edit is refused before QuickJS reads.

TEST_CASE("bytecode: compile errors name the file and write nothing") {
  Sandbox sb;
  const std::string source = sb.write("broken.js", "export default (;");
  const fs::path out = sb.dir / "out.jsbc";
  CHECK(compile_file(source, out.string()) == 1);
  CHECK_FALSE(fs::exists(out));
  CHECK(compile_file((sb.dir / "missing.js").string(), out.string()) == 1);
}

#endif
