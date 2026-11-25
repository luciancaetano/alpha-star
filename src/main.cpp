#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "core/cli.hpp"
#include "core/fixed_step.hpp"
#include "core/version.hpp"
#include "platform/audio_out.hpp"
#include "platform/frame_latency.hpp"
#include "platform/log_file.hpp"
#include "platform/gamepads.hpp"
#include "platform/sdl_input.hpp"
#include "render/renderer.hpp"
#include "script/host.hpp"

#if ALPHA_DEVTOOLS
#include "devtools/dev_tools.hpp"
#endif

// Composition root: alpha_star <pack-dir | game.js> [options]  (pack = `alpha export` output)
// With no game argument it runs the pack it sits in (standalone export).
// Flags below exist only with ALPHA_DEBUG_TOOLS (see cmake/Flavors.cmake); every
// unknown flag is ignored, and shipping builds know none.
// alpha_star --test <tests.js> [--assets DIR]: runs every exported test function headlessly
// (no window, no audio), one PASS/FAIL line each; exit code 0 iff all pass.
//   --frames N        run exactly N frames, one fixed step each (deterministic)
//   --screenshot PATH save the last frame as BMP (with --frames)
//   --tap INPUT       input name to tap headlessly, e.g. Space
//   --tap-every K     tap it every K frames
//   --devtools        open the dev tools (F1) at start; dev builds only
namespace {

#if ALPHA_DEVTOOLS
using Tools = alpha::devtools::DevTools;
using alpha::devtools::FrameStats;
#else
// Builds without ALPHA_DEVTOOLS have no dev tools: the loop calls these no-ops instead.
struct FrameStats {
  double frame_ms = 0.0;
  int steps = 0;
};
struct Tools {
  Tools(SDL_Window*, SDL_Renderer*) {}
  void toggle() {}
  bool handle(const SDL_Event&) { return true; }
  int steps(int wanted) { return wanted; }
  bool paused() const { return false; }
  void decorate(alpha::render::Frame&, const alpha::ecs::World*) const {}
  void draw(alpha::script::Host&, FrameStats) {}
};
#endif

constexpr double kDt = 1.0 / 60.0;
constexpr float kWindowScale = 1.5f;

using Options = alpha::core::CliOptions;

// Standalone pack (`alpha export --standalone`): main.js next to the executable.
std::string fused_pack() {
  const char* base = SDL_GetBasePath();
  if (!base || !std::filesystem::exists(std::filesystem::path(base) / "main.js")) return "";
  return base;
}

int fail(const char* what) {
  SDL_Log("%s failed: %s", what, SDL_GetError());
  SDL_Quit();
  return 1;
}

// Device state between steps and the open gamepads (their player slots).
struct Devices {
  alpha::platform::InputState input;
  alpha::platform::Gamepads pads;
};

// Controllers plugged in or out while running (also the ones present at start).
void hot_plug(Devices& d, const SDL_Event& event) {
  if (const auto gone = d.pads.handle(event)) alpha::platform::release_player(d.input, *gone);
}

// Routing: dev tools first (F1, their windows), then the game UI (in game
// pixels), and only what neither consumed drives game actions.
bool pump(Devices& d, Tools& tools, alpha::script::Host& host, SDL_Renderer* sdl) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) return false;
    hot_plug(d, event);
    if (!tools.handle(event)) continue;
    SDL_Event logical = event;
    SDL_ConvertEventToRenderCoordinates(sdl, &logical);
    if (host.reaches_game(logical)) alpha::platform::apply_event(d.input, event, d.pads.slots());
  }
  return true;
}

void tap(const Options& o, int frame, alpha::platform::InputState& input) {
  const auto code = alpha::core::parse_input(o.tap);
  if (code && o.tap_every > 0 && frame % o.tap_every == 0) input.pressed[0].set(*code);
}

double seconds_since(Uint64& last) {
  const Uint64 now = SDL_GetTicksNS();
  const double elapsed = static_cast<double>(now - last) / 1e9;
  last = now;
  return elapsed;
}

// Exactly 1 step per frame in --frames mode, real time otherwise; dev tools may pause.
int steps_for_frame(const Options& o, alpha::core::FixedStep& clock, Tools& tools, double elapsed) {
  return tools.steps(o.frames > 0 ? 1 : clock.advance(elapsed));
}

// How far into the next fixed step to draw (smooth motion at any refresh rate).
// Exact state when stepping frame by frame (--frames, dev tools pause).
float blend_for(const Options& o, const alpha::core::FixedStep& clock, const Tools& tools) {
  if (o.frames > 0 || tools.paused()) return 1.0f;
  return static_cast<float>(clock.fraction());
}

// Game frame, then dev overlays drawn with it (physics view), then the dev tools windows.
void draw(alpha::script::Host& host, alpha::render::Renderer& renderer, Tools& tools, FrameStats stats,
          float blend) {
  alpha::render::Frame frame = host.frame(blend);
  tools.decorate(frame, host.world());
  renderer.draw(frame);
  host.draw_ui(renderer.sdl(), static_cast<double>(SDL_GetTicksNS()) / 1e9);
  tools.draw(host, stats);
}

void run(const Options& o, alpha::script::Host& host, alpha::render::Renderer& renderer, Tools& tools) {
  alpha::platform::AudioOut audio;
  if (!audio.ok()) SDL_Log("no audio device (%s); running silent", SDL_GetError());
  Devices d;
  alpha::core::FixedStep clock(kDt, 5);
  Uint64 last = SDL_GetTicksNS();
  bool reported = false;
  for (int frame = 0; pump(d, tools, host, renderer.sdl()) && (o.frames == 0 || frame < o.frames); ++frame) {
    tap(o, frame, d.input);
    const double elapsed = seconds_since(last);
    const int steps = steps_for_frame(o, clock, tools, elapsed);
    for (int s = 0; s < steps; ++s) host.step(alpha::platform::consume(d.input, d.pads.slots()), static_cast<float>(kDt));
    d.pads.rumble(host.take_rumble());
    audio.pump(host.mixer());
    draw(host, renderer, tools, FrameStats{elapsed * 1000.0, steps}, blend_for(o, clock, tools));
    if (host.paused() && !reported) reported = std::fprintf(stderr, "%s\n", host.error().c_str()) > 0;
    if (!o.screenshot.empty() && frame + 1 == o.frames) renderer.save_bmp(o.screenshot);
    renderer.present();
  }
}

#if ALPHA_DEBUG_TOOLS
// Error text indented under its FAIL line, without trailing blank lines.
std::string indented(const std::string& text) {
  std::string out;
  for (const char c : text.substr(0, text.find_last_not_of("\r\n") + 1)) {
    out += c == '\n' ? std::string("\n    ") : std::string(1, c);
  }
  return out;
}

int run_tests(const std::string& path, const std::string& asset_dir) {
  alpha::script::Host host([](const std::string&) { return std::string(); });
  const auto results = host.test(path, asset_dir);
  std::size_t failed = 0;
  for (const auto& r : results) {
    failed += r.passed() ? 0U : 1U;
    if (r.passed()) std::printf("PASS %s\n", r.name.c_str());
    else std::printf("FAIL %s\n    %s\n", r.name.c_str(), indented(r.error).c_str());
  }
  if (results.empty()) std::printf("no tests: %s exports no functions\n", path.c_str());
  std::printf("%zu passed, %zu failed\n", results.size() - failed, failed);
  return failed == 0 && !results.empty() ? 0 : 1;
}

#endif

// No game argument and no fused pack: tell the developer (usage) or the player.
int no_game() {
#if ALPHA_DEBUG_TOOLS
  std::fprintf(stderr,
               "usage: alpha_star <pack-dir | game.js> [--frames N] [--screenshot out.bmp] [--tap INPUT] "
               "[--tap-every K] [--devtools]\n       alpha_star --test <tests.js> [--assets DIR]\n");
#else
  const char* text = "No game found. Put this program next to the game's main.js (alpha export --standalone).";
  SDL_Log("%s", text);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "alpha-star", text, nullptr);
#endif
  return 2;
}

// VSync with the shortest frame queue the driver allows (input latency).
void limit_latency(SDL_Renderer* sdl) {
  SDL_SetRenderVSync(sdl, 1);
  [[maybe_unused]] const bool short_queue = alpha::platform::limit_frame_queue(sdl);
#if ALPHA_DEBUG_TOOLS  // diagnostics
  SDL_Log("renderer %s, frame queue: %s", SDL_GetRendererName(sdl), short_queue ? "1 frame" : "driver default");
#endif
}

std::string save_path_for(const std::string& title) { return alpha::platform::pref_file(title, "save.json"); }

}  // namespace

int main(int argc, char** argv) {
  const std::vector<std::string_view> args(argv + 1, argv + argc);
  const Options options = alpha::core::parse_cli(args);
#if ALPHA_DEBUG_TOOLS
  if (!options.test_file.empty()) return run_tests(options.test_file, options.test_assets);
#endif
  const std::string game = options.game.empty() ? fused_pack() : options.game;
  if (game.empty()) return no_game();
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) return fail("SDL_Init");
  alpha::script::Host host(save_path_for);
  const alpha::script::GameInfo info = host.load(game);
#if ALPHA_SHIPPING  // no console: logs go to <pref dir>/log.txt
  alpha::platform::redirect_logs(alpha::platform::pref_file(info.title, "log.txt"), host.log());
#endif
  SDL_Log("alpha-star %s", alpha::core::version().data());
  SDL_Window* window = nullptr;
  SDL_Renderer* sdl = nullptr;
  const auto w = static_cast<int>(static_cast<float>(info.width) * kWindowScale);
  const auto h = static_cast<int>(static_cast<float>(info.height) * kWindowScale);
  if (!SDL_CreateWindowAndRenderer(info.title.c_str(), w, h, SDL_WINDOW_RESIZABLE, &window, &sdl)) {
    return fail("SDL_CreateWindowAndRenderer");
  }
  limit_latency(sdl);
  alpha::render::Renderer renderer(sdl, info.width, info.height);
  {
    Tools tools(window, sdl);  // gone before its renderer
    if (options.devtools) tools.toggle();
    run(options, host, renderer, tools);
  }
  host.release_ui();
  SDL_DestroyRenderer(sdl);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
