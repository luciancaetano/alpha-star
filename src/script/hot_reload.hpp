#pragma once

#include <memory>

#include "script/host.hpp"

#if ALPHA_DEBUG_TOOLS
#include "core/pack_watch.hpp"
#endif

namespace alpha::script {

#if ALPHA_DEBUG_TOOLS
// The main loop's file watch: when anything under the game's pack directory
// changes (and then stays quiet), reloads the game (Host::reload). Results go
// to stderr; a failed reload also shows over the still-running old game.
class HotReload {
 public:
  // enabled = false: never watches (deterministic --frames runs).
  HotReload(const Host& host, bool enabled);
  // Once per frame; now: seconds on a monotonic clock.
  void poll(Host& host, double now);

 private:
  std::unique_ptr<core::PackWatch> watch_;
};
#else
// Shipping builds have no hot reload: the loop calls this no-op instead.
class HotReload {
 public:
  HotReload(const Host&, bool) {}
  void poll(Host&, double) {}
};
#endif

}  // namespace alpha::script
