#pragma once

#include <SDL3/SDL_iostream.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// The engine's only door to files (tools/check_file_access.py keeps it so).
// Built on SDL_IOStream / SDL filesystem calls, so the same code reads a pack
// from a folder, from an Android APK's assets (SDL_IOFromFile does) or, later,
// from an archive. Every read loads the whole file and closes it at once: no
// file stays open while the game runs (Windows would refuse to replace it,
// e.g. a hot reload re-exporting the pack). Decoders read from memory.
namespace alpha::platform::vfs {

using Bytes = std::vector<std::uint8_t>;

// The whole file; nullopt when it does not exist, is a directory or cannot be read.
std::optional<Bytes> read(const std::filesystem::path& path);
std::optional<std::string> read_text(const std::filesystem::path& path);

struct Info {
  std::uint64_t size = 0;
  std::int64_t modified = 0;  // SDL_Time (ns); 0 where the platform does not say
  bool directory = false;
};
// Nothing for a missing path. Inside an APK only exists() is reliable.
std::optional<Info> info(const std::filesystem::path& path);
bool exists(const std::filesystem::path& path);
bool is_directory(const std::filesystem::path& path);

// Every regular file below `dir` (recursive), keyed by its path relative to
// `dir` with '/' separators. Empty when `dir` is missing. Never throws.
std::map<std::string, Info> files_under(const std::filesystem::path& dir);

enum class Backup { keep, none };
// Writes `data` so a crash never leaves a half-written file: it goes to
// `path.tmp`, the current file is copied to `path.bak` (Backup::keep), then
// the temp file is renamed over `path`. Throws core::Error naming the file.
void write_atomic(const std::filesystem::path& path, std::string_view data, Backup backup = Backup::keep);

struct IoCloser {
  void operator()(SDL_IOStream* io) const { SDL_CloseIO(io); }
};
using Io = std::unique_ptr<SDL_IOStream, IoCloser>;
// A read-only stream over `bytes` (which must outlive it), for decoders that
// take an SDL_IOStream (images, WAV).
Io memory(const Bytes& bytes);

// What `write` puts into a growing memory stream (encoders: PNG, BMP);
// nullopt when it returns false.
std::optional<std::string> written(const std::function<bool(SDL_IOStream*)>& write);

// The pack root abstraction: an asset name from a game ("assets/a.png") under
// its pack root. Assets never name absolute paths, so a pack can move into an
// APK or an archive without the game changing.
std::filesystem::path in_pack(const std::filesystem::path& root, const std::string& name);
// The file's extension in lower case (".ogg", ".fnt"; "" without one): picks a decoder.
std::string lower_extension(const std::filesystem::path& path);

// The process's stderr goes to `path` (shipping builds: the log file).
bool redirect_stderr(const std::string& path);

}  // namespace alpha::platform::vfs
