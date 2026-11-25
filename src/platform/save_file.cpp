#include "platform/save_file.hpp"

#include <fstream>
#include <sstream>
#include <system_error>

#include "core/error.hpp"

namespace alpha::platform {

namespace fs = std::filesystem;

std::optional<std::string> read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return std::nullopt;
  std::ostringstream text;
  text << in.rdbuf();
  return text.str();
}

void write_atomic(const fs::path& path, const std::string& text) {
  const fs::path tmp = path.string() + ".tmp";
  {
    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!(out << text) || !out.flush()) throw core::Error("cannot write \"" + path.string() + "\"");
  }
  std::error_code ec;
  if (fs::exists(path, ec)) fs::copy_file(path, path.string() + ".bak", fs::copy_options::overwrite_existing, ec);
  fs::rename(tmp, path, ec);
  if (ec) throw core::Error("cannot replace \"" + path.string() + "\": " + ec.message());
}

}  // namespace alpha::platform
