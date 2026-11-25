#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace alpha::platform {

// The whole file as text; nullopt when it does not exist or cannot be read.
std::optional<std::string> read_file(const std::filesystem::path& path);

// Writes `text` to `path` so a crash never leaves a half-written file: the
// text goes to `path.tmp`, the current file is copied to `path.bak`, then the
// temp file is renamed over `path`. Throws core::Error naming the file.
void write_atomic(const std::filesystem::path& path, const std::string& text);

}  // namespace alpha::platform
