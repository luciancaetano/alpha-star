#include "script/instances.hpp"

#include <string>

#include "core/error.hpp"

namespace alpha::script {

Instances::Instances(std::function<std::string(const std::string&)> save_path_for) {
  auto state = std::make_unique<ScriptState>();
  state->save_path_for = std::move(save_path_for);
  active_ = state.get();
  states_.emplace(kMain, std::move(state));
}

ScriptState& Instances::at(std::uint32_t id) {
  const auto it = states_.find(id);
  if (it == states_.end()) throw core::Error("no game instance #" + std::to_string(id));
  return *it->second;
}

std::uint32_t Instances::create() {
  auto state = std::make_unique<ScriptState>();
  state->asset_dir = main().asset_dir;
  state->save_path_for = [](const std::string&) { return std::string(); };
  states_.emplace(next_id_, std::move(state));
  return next_id_++;
}

std::uint32_t Instances::use(std::uint32_t id) {
  active_ = &at(id);
  const std::uint32_t previous = active_id_;
  active_id_ = id;
  return previous;
}

void Instances::destroy(std::uint32_t id) {
  if (id == kMain || id == active_id_) throw core::Error("cannot destroy the main or the active game instance");
  states_.erase(id);
}

}  // namespace alpha::script
