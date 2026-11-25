#include "script/hot_reload.hpp"

#include <cstdio>

#include "platform/vfs.hpp"

namespace alpha::script {

core::PackWatch::Snapshot pack_snapshot(const std::filesystem::path& dir) {
  core::PackWatch::Snapshot out;
  for (const auto& [name, file] : platform::vfs::files_under(dir)) out[name] = {file.size, file.modified};
  return out;
}

HotReload::HotReload(const Host& host, bool enabled)
    : watch_(enabled ? std::make_unique<core::PackWatch>([dir = host.pack_dir()] { return pack_snapshot(dir); }) : nullptr) {}

void HotReload::poll(Host& host, double now) {
  if (!watch_ || !watch_->poll(now)) return;
  const std::string error = host.reload();
  if (error.empty()) std::fprintf(stderr, "hot reload: %s (scene %s)\n", host.pack_dir().string().c_str(), host.scene().c_str());
  else std::fprintf(stderr, "hot reload: %s\n", error.c_str());
}

}  // namespace alpha::script
