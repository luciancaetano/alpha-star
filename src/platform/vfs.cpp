#include "platform/vfs.hpp"

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>

#include "core/error.hpp"

namespace alpha::platform::vfs {

namespace {

// SDL takes UTF-8 paths on every platform.
std::string utf8(const std::filesystem::path& path) {
  const std::u8string text = path.generic_u8string();
  return {text.begin(), text.end()};
}

struct Freer {
  void operator()(void* p) const { SDL_free(p); }
};

Info info_of(const SDL_PathInfo& p) {
  Info out;
  out.size = p.size;
  out.modified = p.modify_time;
  out.directory = p.type == SDL_PATHTYPE_DIRECTORY;
  return out;
}

bool write_all(const std::string& path, std::string_view data) {
  SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "wb");
  if (!io) return false;
  const bool wrote = SDL_WriteIO(io, data.data(), data.size()) == data.size() && SDL_FlushIO(io);
  return SDL_CloseIO(io) && wrote;
}

[[noreturn]] void fail(const std::string& what, const std::string& path) {
  throw core::Error(what + " \"" + path + "\": " + SDL_GetError());
}

}  // namespace

std::optional<Bytes> read(const std::filesystem::path& path) {
  if (vfs::is_directory(path)) return std::nullopt;  // qualified: std::filesystem's is found by ADL too
  std::size_t size = 0;
  const std::unique_ptr<void, Freer> data(SDL_LoadFile(utf8(path).c_str(), &size));  // opens, reads, closes
  if (!data) return std::nullopt;
  const auto* bytes = static_cast<const std::uint8_t*>(data.get());
  return Bytes(bytes, bytes + size);
}

std::optional<std::string> read_text(const std::filesystem::path& path) {
  const auto bytes = read(path);
  if (!bytes) return std::nullopt;
  return std::string(bytes->begin(), bytes->end());
}

std::optional<Info> info(const std::filesystem::path& path) {
  SDL_PathInfo p{};
  if (!SDL_GetPathInfo(utf8(path).c_str(), &p) || p.type == SDL_PATHTYPE_NONE) return std::nullopt;
  return info_of(p);
}

// Asset files inside an APK have no path info, but they open.
bool exists(const std::filesystem::path& path) {
  if (info(path)) return true;
  const Io io(SDL_IOFromFile(utf8(path).c_str(), "rb"));
  return io != nullptr;
}

bool is_directory(const std::filesystem::path& path) {
  const auto i = info(path);
  return i && i->directory;
}

std::map<std::string, Info> files_under(const std::filesystem::path& dir) {
  std::map<std::string, Info> out;
  int count = 0;
  const std::unique_ptr<char*, Freer> names(SDL_GlobDirectory(utf8(dir).c_str(), nullptr, 0, &count));
  for (int i = 0; names && i < count; ++i) {
    const std::filesystem::path name(std::u8string(names.get()[i], names.get()[i] + std::strlen(names.get()[i])));
    const auto found = info(dir / name);
    if (found && !found->directory) out[utf8(name)] = *found;  // a file vanishing mid-scan is skipped
  }
  return out;
}

void write_atomic(const std::filesystem::path& path, std::string_view data, Backup backup) {
  const std::string target = utf8(path);
  const std::string tmp = target + ".tmp";
  if (!write_all(tmp, data)) fail("cannot write", target);
  if (backup == Backup::keep && info(path)) SDL_CopyFile(target.c_str(), (target + ".bak").c_str());
  if (!SDL_RenamePath(tmp.c_str(), target.c_str())) fail("cannot replace", target);
}

Io memory(const Bytes& bytes) { return Io(SDL_IOFromConstMem(bytes.data(), bytes.size())); }

std::optional<std::string> written(const std::function<bool(SDL_IOStream*)>& write) {
  const Io io(SDL_IOFromDynamicMem());
  if (!io || !write(io.get())) return std::nullopt;
  const auto* mem = static_cast<const char*>(
      SDL_GetPointerProperty(SDL_GetIOProperties(io.get()), SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, nullptr));
  return std::string(mem ? mem : "", mem ? static_cast<std::size_t>(SDL_TellIO(io.get())) : 0);
}

std::filesystem::path in_pack(const std::filesystem::path& root, const std::string& name) {
  const std::filesystem::path rel(std::u8string(name.begin(), name.end()));
  return root.empty() ? rel : root / rel;
}

bool redirect_stderr(const std::string& path) {
#if defined(_WIN32)
  std::FILE* file = nullptr;
  return freopen_s(&file, path.c_str(), "w", stderr) == 0;
#else
  return std::freopen(path.c_str(), "w", stderr) != nullptr;
#endif
}

}  // namespace alpha::platform::vfs
