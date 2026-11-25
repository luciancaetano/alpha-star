#include "devtools/profiler.hpp"

#include <imgui.h>

#include <algorithm>
#include <string>

#include "devtools/panels.hpp"

namespace alpha::devtools {

std::vector<float> frame_totals(const core::PerfHistory& perf) {
  std::vector<float> out;
  out.reserve(perf.size());
  for (std::size_t i = 0; i < perf.size(); ++i) out.push_back(static_cast<float>(perf.at(i).total()));
  return out;
}

namespace {

void section_row(const char* name, double last, const core::Spread& s) {
  ImGui::TableNextRow();
  ImGui::TableNextColumn();
  ImGui::TextUnformatted(name);
  for (const double v : {last, s.p50, s.p95, s.max}) {
    ImGui::TableNextColumn();
    ImGui::Text("%.3f", v);
  }
}

void section_table(const core::PerfHistory& perf) {
  if (!ImGui::BeginTable("sections", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) return;
  for (const char* head : {"ms", "last", "p50", "p95", "max"}) ImGui::TableSetupColumn(head);
  ImGui::TableHeadersRow();
  const core::FrameTimes last = perf.size() ? perf.at(perf.size() - 1) : core::FrameTimes{};
  for (std::size_t i = 0; i < core::kSections; ++i) {
    const auto s = static_cast<core::Section>(i);
    section_row(std::string(core::kSectionNames[i]).c_str(), last[s], perf.spread(s));
  }
  section_row("total", last.total(), perf.total());
  ImGui::EndTable();
}

}  // namespace

void profiler_window(const script::Host& host, const render::RenderStats& drawn, const Rect& at) {
  place_next(at);
  ImGui::Begin("Profiler");
  const core::PerfHistory& perf = host.perf();
  const std::vector<float> totals = frame_totals(perf);
  const float top = std::max(16.7f, totals.empty() ? 0.0f : *std::max_element(totals.begin(), totals.end()));
  ImGui::PlotLines("##frames", totals.data(), static_cast<int>(totals.size()), 0, "frame ms (16.7 = 60 fps)", 0.0f,
                   top, {300, 60});
  section_table(perf);
  const script::JsMemory js = host.js_memory();
  ImGui::Text("JS heap %.0f KB  %.0f objects  next GC at %.0f KB", js.used_bytes / 1024.0, js.objects,
              js.gc_threshold / 1024.0);
  ImGui::Text("draw calls %d  vertices %d  drawn %d  culled %d", drawn.draw_calls, drawn.vertices, drawn.drawn,
              drawn.culled);
  ImGui::End();
}

}  // namespace alpha::devtools
