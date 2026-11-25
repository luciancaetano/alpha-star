#pragma once

#include <vector>

#include "core/perf.hpp"
#include "render/renderer.hpp"
#include "script/host.hpp"

namespace alpha::devtools {

// Frame totals (ms) of the kept history, oldest first: the profiler's graph.
std::vector<float> frame_totals(const core::PerfHistory& perf);

// "Profiler" window: per-section ms (last frame, p50, p95, max), a frame
// time graph, the JS heap and the last frame's draw calls and vertices.
// Must run between ImGui::NewFrame and ImGui::Render.
void profiler_window(const script::Host& host, const render::RenderStats& drawn);

}  // namespace alpha::devtools
