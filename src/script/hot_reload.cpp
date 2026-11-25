#include "script/hot_reload.hpp"

#include <cstdio>

namespace alpha::script {

HotReload::HotReload(const Host& host, bool enabled)
    : watch_(enabled ? std::make_unique<core::PackWatch>(host.pack_dir()) : nullptr) {}

void HotReload::poll(Host& host, double now) {
  if (!watch_ || !watch_->poll(now)) return;
  const std::string error = host.reload();
  if (error.empty()) std::fprintf(stderr, "hot reload: %s (scene %s)\n", host.pack_dir().string().c_str(), host.scene().c_str());
  else std::fprintf(stderr, "hot reload: %s\n", error.c_str());
}

}  // namespace alpha::script
