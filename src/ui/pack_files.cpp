#include "ui/pack_files.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "platform/vfs.hpp"

namespace alpha::ui {

namespace {

struct OpenFile {
  platform::vfs::Bytes bytes;
  size_t at = 0;
};

OpenFile& of(Rml::FileHandle file) { return *reinterpret_cast<OpenFile*>(file); }

}  // namespace

Rml::FileHandle PackFiles::Open(const Rml::String& path) {
  auto bytes = platform::vfs::read(path);
  if (!bytes) return {};
  return reinterpret_cast<Rml::FileHandle>(new OpenFile{std::move(*bytes), 0});
}

void PackFiles::Close(Rml::FileHandle file) { delete &of(file); }

size_t PackFiles::Read(void* buffer, size_t size, Rml::FileHandle file) {
  OpenFile& f = of(file);
  const size_t n = std::min(size, f.bytes.size() - f.at);
  if (n > 0) std::memcpy(buffer, f.bytes.data() + f.at, n);
  f.at += n;
  return n;
}

bool PackFiles::Seek(Rml::FileHandle file, long offset, int origin) {
  OpenFile& f = of(file);
  long base = 0;
  if (origin == SEEK_CUR) base = static_cast<long>(f.at);
  if (origin == SEEK_END) base = static_cast<long>(f.bytes.size());
  const long to = base + offset;
  if (to < 0 || to > static_cast<long>(f.bytes.size())) return false;
  f.at = static_cast<size_t>(to);
  return true;
}

size_t PackFiles::Tell(Rml::FileHandle file) { return of(file).at; }

size_t PackFiles::Length(Rml::FileHandle file) { return of(file).bytes.size(); }

}  // namespace alpha::ui
