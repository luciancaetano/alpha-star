#pragma once

#include <string>

#include "core/perf.hpp"
#include "render/renderer.hpp"
#include "script/js_engine.hpp"

// `--frames N` run reports (ALPHA_DEBUG_TOOLS only; not compiled otherwise).
namespace alpha::script {

class Host;

// What a finished run measured: frame time spreads per section, the last
// frame's drawing, the JS heap and the living entities. One JSON object.
std::string perf_report(const core::PerfHistory& perf, const render::RenderStats& drawn, const JsMemory& js,
                        std::size_t entities);

// Logs the last frame's drawing and, with --perf PATH, writes perf_report there.
void report_run(const std::string& perf_path, Host& host, const render::RenderStats& drawn);

}  // namespace alpha::script
