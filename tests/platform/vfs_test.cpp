#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "core/error.hpp"
#include "platform/vfs.hpp"

using namespace alpha;
namespace vfs = alpha::platform::vfs;
namespace fs = std::filesystem;

namespace {

fs::path fresh_dir(const char* name) {
  const fs::path dir = fs::temp_directory_path() / name;
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

}  // namespace

TEST_CASE("read_text gives the text, or nothing when the file is missing") {
  const fs::path dir = fresh_dir("alpha_vfs_read");
  CHECK_FALSE(vfs::read_text(dir / "none.json").has_value());
  std::ofstream(dir / "a.json", std::ios::binary) << "{\"x\":1}";
  CHECK(vfs::read_text(dir / "a.json") == std::optional<std::string>("{\"x\":1}"));
  const auto bytes = vfs::read(dir / "a.json");
  REQUIRE(bytes.has_value());
  CHECK(bytes->size() == 7);
  CHECK((*bytes)[0] == '{');
  CHECK(vfs::read_text(dir).has_value() == false);  // a directory is not a file
  fs::remove_all(dir);
}

TEST_CASE("an empty file reads as empty, not as missing") {
  const fs::path dir = fresh_dir("alpha_vfs_empty");
  std::ofstream(dir / "e.txt", std::ios::binary).flush();
  CHECK(vfs::read_text(dir / "e.txt") == std::optional<std::string>(""));
  fs::remove_all(dir);
}

TEST_CASE("info: size, kind and a modification time; nothing for a missing path") {
  const fs::path dir = fresh_dir("alpha_vfs_info");
  std::ofstream(dir / "f.bin", std::ios::binary) << "12345";
  const auto file = vfs::info(dir / "f.bin");
  REQUIRE(file.has_value());
  CHECK(file->size == 5);
  CHECK_FALSE(file->directory);
  CHECK(file->modified != 0);
  CHECK(vfs::info(dir)->directory);
  CHECK(vfs::exists(dir / "f.bin"));
  CHECK(vfs::is_directory(dir));
  CHECK_FALSE(vfs::is_directory(dir / "f.bin"));
  CHECK_FALSE(vfs::exists(dir / "nope"));
  CHECK_FALSE(vfs::info(dir / "nope").has_value());
  fs::remove_all(dir);
}

TEST_CASE("write_atomic replaces the file and keeps the previous one as .bak") {
  const fs::path dir = fresh_dir("alpha_vfs_write");
  const fs::path file = dir / "save.json";
  vfs::write_atomic(file, "one");
  CHECK(vfs::read_text(file) == std::optional<std::string>("one"));
  CHECK_FALSE(fs::exists(dir / "save.json.bak"));
  vfs::write_atomic(file, "two");
  CHECK(vfs::read_text(file) == std::optional<std::string>("two"));
  CHECK(vfs::read_text(dir / "save.json.bak") == std::optional<std::string>("one"));
  CHECK_FALSE(fs::exists(dir / "save.json.tmp"));
  vfs::write_atomic(dir / "shot.bmp", "BM", vfs::Backup::none);
  vfs::write_atomic(dir / "shot.bmp", "BM2", vfs::Backup::none);
  CHECK(vfs::read_text(dir / "shot.bmp") == std::optional<std::string>("BM2"));
  CHECK_FALSE(fs::exists(dir / "shot.bmp.bak"));
  fs::remove_all(dir);
}

TEST_CASE("write_atomic names the file when it cannot write") {
  const fs::path missing = fs::temp_directory_path() / "alpha_no_such_dir" / "x" / "save.json";
  fs::remove_all(fs::temp_directory_path() / "alpha_no_such_dir");
  CHECK_THROWS_WITH_AS(vfs::write_atomic(missing, "x"), doctest::Contains("save.json"), core::Error);
}

TEST_CASE("files_under lists every file below a directory, relative with '/', sizes included") {
  const fs::path dir = fresh_dir("alpha_vfs_list");
  fs::create_directories(dir / "assets" / "deep");
  std::ofstream(dir / "main.js", std::ios::binary) << "1";
  std::ofstream(dir / "assets" / "deep" / "a.png", std::ios::binary) << "123";
  const auto files = vfs::files_under(dir);
  REQUIRE(files.size() == 2);
  CHECK(files.at("main.js").size == 1);
  CHECK(files.at("assets/deep/a.png").size == 3);
  CHECK(vfs::files_under(dir / "missing").empty());
  fs::remove_all(dir);
}

TEST_CASE("a file read through the layer is closed again: it can be deleted at once") {
  const fs::path dir = fresh_dir("alpha_vfs_closed");
  std::ofstream(dir / "x.txt", std::ios::binary) << "x";
  CHECK(vfs::read(dir / "x.txt").has_value());
  std::error_code ec;
  CHECK(fs::remove(dir / "x.txt", ec));
  fs::remove_all(dir);
}

TEST_CASE("memory streams read the bytes they were given") {
  const vfs::Bytes bytes{'a', 'b', 'c'};
  const vfs::Io io = vfs::memory(bytes);
  REQUIRE(io);
  char out[3] = {};
  CHECK(SDL_ReadIO(io.get(), out, 3) == 3);
  CHECK(std::string(out, 3) == "abc");
}

TEST_CASE("written collects what an encoder writes; nothing when it fails") {
  const auto text = vfs::written([](SDL_IOStream* io) { return SDL_WriteIO(io, "png!", 4) == 4; });
  CHECK(text == std::optional<std::string>("png!"));
  CHECK_FALSE(vfs::written([](SDL_IOStream*) { return false; }).has_value());
}

TEST_CASE("pack paths: asset names resolve under the pack root with '/'") {
  CHECK(vfs::in_pack("game", "assets/a.png") == fs::path("game/assets/a.png"));
  CHECK(vfs::in_pack("", "a.png") == fs::path("a.png"));
}
