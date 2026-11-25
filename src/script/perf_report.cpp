#include "script/perf_report.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdio>

#include "platform/vfs.hpp"
#include "script/host.hpp"

namespace alpha::script {

std::string perf_report(const core::PerfHistory& perf, const render::RenderStats& drawn, const JsMemory& js,
                        std::size_t entities) {
  std::array<char, 256> tail{};
  std::snprintf(tail.data(), tail.size(),
                ",\"drawCalls\":%d,\"vertices\":%d,\"drawn\":%d,\"culled\":%d,\"jsBytes\":%.0f,\"jsObjects\":%.0f,"
                "\"entities\":%zu}",
                drawn.draw_calls, drawn.vertices, drawn.drawn, drawn.culled, js.used_bytes, js.objects, entities);
  return "{\"perf\":" + core::to_json(perf) + tail.data();
}

void report_run(const std::string& perf_path, Host& host, const render::RenderStats& drawn) {
  SDL_Log("render: %d drawn, %d culled, %d draw calls, %d vertices (last frame)", drawn.drawn, drawn.culled,
          drawn.draw_calls, drawn.vertices);
  if (perf_path.empty()) return;
  try {
    platform::vfs::write_atomic(perf_path, perf_report(host.perf(), drawn, host.js_memory(), host.entity_count()),
                                platform::vfs::Backup::none);
  } catch (const std::exception& e) {
    SDL_Log("--perf %s: %s", perf_path.c_str(), e.what());
  }
}

}  // namespace alpha::script
