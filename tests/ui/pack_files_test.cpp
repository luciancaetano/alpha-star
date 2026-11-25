#include <doctest/doctest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "ui/pack_files.hpp"

namespace fs = std::filesystem;

TEST_CASE("RmlUi files come from the file layer, already closed on disk") {
  const fs::path dir = fs::temp_directory_path() / "alpha_pack_files";
  fs::remove_all(dir);
  fs::create_directories(dir);
  std::ofstream(dir / "doc.rml", std::ios::binary) << "<rml>hi</rml>";
  alpha::ui::PackFiles files;
  CHECK_FALSE(files.Open((dir / "missing.rml").string()));
  const Rml::FileHandle f = files.Open((dir / "doc.rml").string());
  REQUIRE(f);
  CHECK(fs::remove(dir / "doc.rml"));  // nothing holds the file
  CHECK(files.Length(f) == 13);
  char buf[6] = {};
  CHECK(files.Read(buf, 5, f) == 5);
  CHECK(std::string(buf, 5) == "<rml>");
  CHECK(files.Tell(f) == 5);
  CHECK(files.Seek(f, -6, SEEK_END));
  CHECK(files.Read(buf, 6, f) == 6);
  CHECK(std::string(buf, 6) == "</rml>");
  CHECK(files.Read(buf, 6, f) == 0);
  CHECK(files.Seek(f, 1, SEEK_SET));
  CHECK(files.Seek(f, 1, SEEK_CUR));
  CHECK(files.Tell(f) == 2);
  CHECK_FALSE(files.Seek(f, -1, SEEK_SET));
  CHECK_FALSE(files.Seek(f, 100, SEEK_SET));
  files.Close(f);
  fs::remove_all(dir);
}
