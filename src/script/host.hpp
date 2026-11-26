#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <filesystem>

#include "core/controls.hpp"
#include "core/log_ring.hpp"
#include "core/perf.hpp"
#include "core/window.hpp"
#include "render/renderer.hpp"
#include "script/vm.hpp"

namespace alpha::script {

struct GameInfo {
  std::string title = "alpha-star";
  int width = 640;
  int height = 360;
  core::Color background{0, 0, 0, 255};
  core::WindowConfig window;  // config.window
  bool pause_on_focus_loss = true;  // config.pauseOnFocusLoss
  bool shaders = false;              // assets.shaders: the window asks for the GPU renderer
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
  core::Controls& controls() { return vm_->main().controls; }
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

  // Hot reload: boots the game again from the file load() was given (fresh
  // script engine, every asset reread) and swaps it in only if it booted, then
  // the old one is dropped. Kept: the save (a file), the current scene with
  // its params (when JSON-safe; else the start scene), and ctx.state when it
  // round-trips through JSON and the new scene still declares state (else
  // state(params) runs again). The world is rebuilt by the scene's enter.
  // Returns "" on success; otherwise the error, logged and shown over the game
  // (reload_error) while the previous game keeps running untouched.
  std::string reload();
  const std::string& reload_error() const { return reload_error_; }
  // The directory the game was loaded from: what a file watch should watch.
  const std::filesystem::path& pack_dir() const { return pack_dir_; }
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

  // ctx.window: the engine applies its fullscreen request and writes the
  // real state every frame (platform::sync_window).
  core::WindowState& window() { return vm_->main().window; }
  const GameInfo& info() const { return info_; }
  // The screen shows `size` game pixels now (scaling "expand"): the scene
  // camera, later scenes and the UI follow.
  void resize_view(core::Vec2 size);

  // Audio produced by the game; the platform layer drains it every frame.
  core::Mixer& mixer() { return vm_->main().mixer; }
  // ctx.locale of the game; empty without assets.locales.
  const std::string& locale() const { return vm_->main().locale; }
  bool paused() const { return !error_.empty(); }
  // The app paused (focus lost, minimized, background) or resumed
  // (script/app_events): ctx.app.paused follows and the running scene's
  // ctx.onPause / ctx.onResume hooks run. A hook's error pauses the game.
  void set_app_paused(bool paused);
  bool app_paused() const { return app_paused_; }
  // The OS is low on memory: logged, then the scene's ctx.onLowMemory hooks run.
  void low_memory();
  const std::string& error() const { return error_; }

  // Read-only views for the engine's dev tools (never exposed to scripts).
  // Script console.* lines and pausing errors, newest last.
  const core::LogRing& log() const;
  // Current scene name; empty before the first scene starts.
  std::string scene();
  // The running scene's world; null before the first scene starts.
  ecs::World* world();
  // The game's UI; null before boot.
  ui::Ui* ui() { return vm_->main().ui.get(); }
  const core::TagRegistry& tags();
  // Wall time of the last step's script call (compare with kCallBudget).
  double last_call_ms() const { return step_ms_; }

  // Profiler: the frame being measured (steps and UI add to it; the main
  // loop adds render and audio), then end_frame() keeps it in the history.
  core::FrameTimes& frame_times() { return frame_times_; }
  void end_frame();
  const core::PerfHistory& perf() const { return perf_; }
  // JS heap of the running game, and its living entities (0 without a scene).
  JsMemory js_memory() const { return vm_->engine->memory(); }
  void collect_garbage() { vm_->engine->collect_garbage(); }
  std::size_t entity_count();

 private:
  // Boots the game at `path` (main.js) in `vm`; `from`: carried scene JSON or "".
  static GameInfo boot(Vm& vm, const std::string& path, const std::string& from);
#if ALPHA_DEBUG_TOOLS
  std::string carried_scene();
  std::vector<TestResult> run_tests(const std::string& path, const std::string& asset_dir);
#endif
  void pause(const std::exception& e);
  void collect_times();
  // Calls runtime export `name`(arg) unless a script error paused the game.
  void notify(const char* name, JSValue arg);

  SavePathFor save_path_for_;
  core::LogRing log_{200};  // outlives every Vm: lines survive a reload
  std::unique_ptr<Vm> vm_;
  std::string main_path_;  // the game's main.js (or .js module)
  std::filesystem::path pack_dir_;
  GameInfo info_;
  std::string error_;
  double step_ms_ = 0.0;
  bool app_paused_ = false;
  core::FrameTimes frame_times_;
  core::PerfHistory perf_{600};  // 10 s at 60 fps
#if ALPHA_DEBUG_TOOLS
  std::string reload_error_;
#endif
};

}  // namespace alpha::script
