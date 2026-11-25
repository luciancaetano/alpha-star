#include "devtools/panels.hpp"

#include <imgui.h>

#include <array>
#include <string>

#include "core/index_of.hpp"
#include "devtools/inspector.hpp"

namespace alpha::devtools {

namespace {

constexpr ImVec4 kErrorColor{1.0f, 0.4f, 0.4f, 1.0f};
constexpr std::array<ImVec4, 3> kLevelColors{{{0.85f, 0.85f, 0.85f, 1.0f}, {1.0f, 0.8f, 0.3f, 1.0f}, kErrorColor}};

std::string joined(const std::vector<std::string>& tags) { return core::join(tags, ", "); }

void overlay_window(const DevSnapshot& s) {
  ImGui::SetNextWindowPos({8, 8}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  ImGui::Text("%.0f fps  %.2f ms  %d steps", s.fps, s.frame_ms, s.steps);
  ImGui::Text("script call %.2f / %.0f ms", s.call_ms, s.budget_ms);
  ImGui::Text("entities %zu  voices %zu", s.entities, s.voices);
  ImGui::Text("scene %s", s.scene.c_str());
  ImGui::End();
}

void time_window(Panels& p, TimeControl& time) {
  ImGui::SetNextWindowPos({8, 112}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Time", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
  if (ImGui::Button(time.paused() ? "Resume" : "Pause")) time.toggle_pause();
  ImGui::SameLine();
  ImGui::BeginDisabled(!time.paused());
  if (ImGui::Button("Step")) time.request_step();
  ImGui::EndDisabled();
  ImGui::Checkbox("physics view (colliders)", &p.physics_view);
  ImGui::End();
}

void entity_editor(ecs::World& world, EntityView v) {
  ImGui::SeparatorText("selected");
  bool changed = ImGui::DragFloat2("pos", &v.pos.x);
  changed |= ImGui::DragFloat("rotation", &v.rotation);
  if (v.velocity) changed |= ImGui::DragFloat2("velocity", &v.velocity->x);
  ImGui::Text("id %u  z %d  tags %s", v.id, v.z, joined(v.tags).c_str());
  if (changed) apply(world, v);
}

void entity_list(Panels& p, ecs::World& world, const core::TagRegistry& tags) {
  const auto views = list_entities(world, tags, p.filter.data());
  ImGui::BeginChild("list", {0, 140}, ImGuiChildFlags_Borders);
  for (const EntityView& v : views) {
    const std::string label = std::to_string(v.id) + "  " + joined(v.tags);
    if (ImGui::Selectable(label.c_str(), p.selected == v.id)) p.selected = v.id;
  }
  ImGui::EndChild();
  for (const EntityView& v : views) {
    if (p.selected == v.id) entity_editor(world, v);
  }
}

void inspector_window(Panels& p, script::Host& host) {
  ImGui::SetNextWindowPos({8, 190}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({300, 300}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Entities");
  ImGui::InputText("tag filter", p.filter.data(), p.filter.size());
  if (ecs::World* world = host.world()) entity_list(p, *world, host.tags());
  ImGui::End();
}

void console_window(const script::Host& host) {
  const ImVec2 screen = ImGui::GetIO().DisplaySize;  // bottom strip: fits narrow windows too
  ImGui::SetNextWindowPos({8, screen.y - 208}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({screen.x - 16, 200}, ImGuiCond_FirstUseEver);
  ImGui::Begin("Console");
  if (host.paused()) ImGui::TextColored(kErrorColor, "paused: %s", host.error().c_str());
  ImGui::BeginChild("log");
  const core::LogRing& log = host.log();
  for (std::size_t i = 0; i < log.size(); ++i) {
    const core::LogLine& line = log.at(i);
    ImGui::TextColored(kLevelColors.at(static_cast<std::size_t>(line.level)), "%s", line.text.c_str());
  }
  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
  ImGui::EndChild();
  ImGui::End();
}

}  // namespace

void draw_panels(Panels& panels, TimeControl& time, script::Host& host, const DevSnapshot& snap) {
  overlay_window(snap);
  time_window(panels, time);
  inspector_window(panels, host);
  console_window(host);
}

}  // namespace alpha::devtools
