#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "core/input.hpp"
#include "render/renderer.hpp"
#include "script/js_engine.hpp"
#include "script/script_state.hpp"

namespace alpha::script {

// Where ctx.save persists (a JSON file), chosen from the game title once the
// game has booted far enough to know it.
using SavePathFor = std::function<std::string(const std::string& title)>;

struct GameInfo {
  std::string title = "alpha-star";
  int width = 640;
  int height = 360;
  core::Color background{0, 0, 0, 255};
};

// Runs one compiled game (a single ES module whose default export comes from
// `game(...)`). Any script error pauses the game: step() stops advancing and
// frame() shows the error. The engine itself never throws out of here.
class Host {
 public:
  explicit Host(SavePathFor save_path_for);
  ~Host();
  Host(const Host&) = delete;
  Host& operator=(const Host&) = delete;

  // Loads and boots a game: a pack directory (runs <dir>/main.js) or a .js
  // module. Asset paths resolve against the module's directory.
  // On failure returns defaults and pauses.
  GameInfo load(const std::string& path);
  // One fixed step with the inputs held (or tapped) during it.
  void step(const core::InputSet& held, float dt);
  render::Frame frame();

  // Audio produced by the game; the platform layer drains it every frame.
  core::Mixer& mixer() { return state_.mixer; }
  bool paused() const { return !error_.empty(); }
  const std::string& error() const { return error_; }

 private:
  GameInfo boot(const std::string& target);
  void pause(const std::exception& e);
  std::vector<core::TextItem> texts();

  ScriptState state_;
  std::unique_ptr<Engine> engine_;
  std::optional<Value> step_fn_;
  std::optional<Value> texts_fn_;
  GameInfo info_;
  std::string error_;
};

}  // namespace alpha::script
