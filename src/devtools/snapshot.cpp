#include "devtools/snapshot.hpp"

#include <chrono>

namespace alpha::devtools {

DevSnapshot snapshot(script::Host& host, FrameStats stats) {
  const ecs::World* world = host.world();
  return {stats.frame_ms > 0.0 ? 1000.0 / stats.frame_ms : 0.0,
          stats.frame_ms,
          stats.steps,
          host.last_call_ms(),
          std::chrono::duration<double, std::milli>(script::kCallBudget).count(),
          world ? world->entities().size() : 0,
          host.mixer().voices(),
          host.scene(),
          host.error()};
}

}  // namespace alpha::devtools
