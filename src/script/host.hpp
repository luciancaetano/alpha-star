#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/controls.hpp"
#include "core/log_ring.hpp"
#include "render/renderer.hpp"
#include "core/source_map.hpp"
#include "script/instances.hpp"
#include "script/js_engine.hpp"

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

// One exported test function of a test module; error is empty when it passed.
struct TestResult {
  std::string name;
  std::string error;
  bool passed() const { return error.empty(); }
};

// Runs one compiled game (a single ES module whose default export comes from
// `game(...)`). Any script error pauses the game: step() stops advancing and
// frame() shows the error. The engine itself never throws out of here.
// Error locations in the bundle are rewritten to the TypeScript source via the
// bundle's inline source map.
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
  void step(const core::InputFrame& input, float dt);
  // The game's actions and axes (agent control injects through them).
  core::Controls& controls() { return instances_.main().controls; }
  // Gamepad vibrations the game asked for since the last call (ctx.input.rumble).
  core::RumbleRequests take_rumble();
  // blend: how far real time is into the next fixed step (FixedStep::fraction);
  // 1 draws the current state exactly.
  render::Frame frame(float blend = 1.0f);
#if ALPHA_DEBUG_TOOLS
  // Test mode (no game booted): imports a test module and calls each exported
  // function once, in name order. A module that fails to load is one failure.
  // Game assets resolve against `asset_dir` (default: the module's directory).
  std::vector<TestResult> test(const std::string& path, const std::string& asset_dir = "");
#endif

  // Game UI (RmlUi). Events go to the UI first: false when the UI consumed the
  // event (ui/input.hpp rule), so it must not drive game input. Mouse
  // coordinates in game pixels.
  bool reaches_game(const SDL_Event& event);
  // Draws the UI over the game frame (not while paused: the error overlay
  // shows instead); `now` (seconds) drives held-gamepad focus repeat.
  void draw_ui(SDL_Renderer* sdl, double now);
  // Frees the UI's textures; call before destroying the SDL renderer.
  void release_ui();

  // Audio produced by the game; the platform layer drains it every frame.
  core::Mixer& mixer() { return instances_.main().mixer; }
  bool paused() const { return !error_.empty(); }
  const std::string& error() const { return error_; }

  // Read-only views for the engine's dev tools (never exposed to scripts).
  // Script console.* lines and pausing errors, newest last.
  const core::LogRing& log() const;
  // Current scene name; empty before the first scene starts.
  std::string scene();
  // The running scene's world; null before the first scene starts.
  ecs::World* world();
  // The game's UI; null before boot.
  ui::Ui* ui() { return instances_.main().ui.get(); }
  const core::TagRegistry& tags();
  // Wall time of the last step's script call (compare with kCallBudget).
  double last_call_ms() const { return step_ms_; }

 private:
  GameInfo boot(const std::string& target);
  // Imports the user's bundle (game or tests) and remembers its source map.
  Value import_bundle(const std::string& path);
#if ALPHA_DEBUG_TOOLS
  std::vector<TestResult> run_tests(const std::string& path, const std::string& asset_dir);
#endif
  // An error message with bundle locations mapped to the original sources.
  std::string located(const std::exception& e) const;
  void pause(const std::exception& e);

  Instances instances_;
  std::unique_ptr<Engine> engine_;
  std::string bundle_path_;
  std::optional<core::SourceMap> source_map_;
  std::optional<Value> step_fn_;
  std::optional<Value> scene_fn_;
  GameInfo info_;
  std::string error_;
  double step_ms_ = 0.0;
};

}  // namespace alpha::script
