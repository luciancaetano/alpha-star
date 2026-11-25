#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "core/cli.hpp"
#include "core/fixed_step.hpp"
#include "core/version.hpp"
#include "platform/audio_out.hpp"
#include "platform/frame_latency.hpp"
#include "platform/log_file.hpp"
#include "platform/mobile.hpp"
#include "platform/gamepads.hpp"
#include "platform/sdl_input.hpp"
#include "platform/window.hpp"
#include "render/renderer.hpp"
#include "script/app_events.hpp"
#include "script/host.hpp"
#include "script/hot_reload.hpp"

#if ALPHA_DEVTOOLS
#include "devtools/dev_tools.hpp"
#endif
#if ALPHA_MCP
#include "mcp/control.hpp"
#endif
#if ALPHA_DEBUG_TOOLS
#include "script/bytecode.hpp"
#include "script/perf_report.hpp"
#include "script/test_report.hpp"
#endif

// Composition root: alpha_star <pack-dir | game.js> [options]  (pack = `alpha export` output)
// With no game argument it runs the pack it sits in (standalone export: game.jsbc).
// alpha_star --compile <main.js> <out.jsbc> (ALPHA_DEBUG_TOOLS): bytecode for standalone packs.
// Flags below exist only with ALPHA_DEBUG_TOOLS (see cmake/Flavors.cmake); every
// unknown flag is ignored, and shipping builds know none.
// alpha_star --test <tests.js> [--assets DIR]: runs every exported test function headlessly
// (no window, no audio), one PASS/FAIL line each; exit code 0 iff all pass.
//   --frames N        run exactly N frames, one fixed step each (deterministic)
//   --screenshot PATH save the last frame as BMP (with --frames)
//   --tap INPUT       input name to tap headlessly, e.g. Space
//   --tap-every K     tap it every K frames
//   --devtools        open the dev tools (F12) at start; dev builds only
//   --version         print "alpha-star <version>" and exit
// With ALPHA_MCP (dev, release): agent control channel, see src/mcp/dispatch.hpp
//   --control PORT    serve the alpha-control protocol on 127.0.0.1:PORT
//   --hidden          no visible window (agent runs)
// With ALPHA_DEBUG_TOOLS (dev, release) the game hot-reloads when its pack changes
// (script/hot_reload.hpp); the control channel has a `reload` command too.
namespace {

#if ALPHA_DEVTOOLS
using Tools = alpha::devtools::DevTools;
using alpha::devtools::FrameStats;
#else
// Builds without ALPHA_DEVTOOLS have no dev tools: the loop calls these no-ops instead.
struct FrameStats {
  double frame_ms = 0.0;
  int steps = 0;
  alpha::render::RenderStats drawn;
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
using InputSource = std::function<alpha::core::InputFrame()>;

#if ALPHA_MCP
using Control = alpha::mcp::Control;
Control make_control(const alpha::core::CliOptions& o, alpha::script::Host& host, alpha::render::Renderer& r,
                     InputSource devices) {
  return Control(alpha::mcp::control_config(o), host, r, std::move(devices), static_cast<float>(kDt));
}
#else
// Builds without ALPHA_MCP have no agent channel: every step is a device step.
struct Control {
  alpha::script::Host& host;
  InputSource devices;
  int steps(int wanted) const { return wanted; }
  bool paused() const { return false; }
  void step() { host.step(devices(), static_cast<float>(kDt)); }
  bool poll(double) { return true; }
};
Control make_control(const alpha::core::CliOptions&, alpha::script::Host& host, alpha::render::Renderer&,
                     InputSource devices) {
  return Control{host, std::move(devices)};
}
#endif

using Options = alpha::core::CliOptions;

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

// Routing: dev tools first (F12, their windows), then the game UI (in game
// pixels), and only what neither consumed drives game actions.
bool pump(Devices& d, Tools& tools, alpha::script::Host& host, SDL_Renderer* sdl, alpha::script::AppEvents& app) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) return false;
    hot_plug(d, event);
    app.handle(event, host);  // focus/minimize/background: may pause (events still flow on)
    if (alpha::platform::window_event(event, host.info().window, host.window())) continue;
    if (!tools.handle(event)) continue;
    SDL_Event logical = event;
    SDL_ConvertEventToRenderCoordinates(sdl, &logical);
    if (!host.reaches_game(logical)) continue;
    alpha::platform::apply_event(d.input, logical, d.pads.slots());
    alpha::platform::apply_touch(d.input, event);  // window-relative x: TouchLeft / TouchRight
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

// A player at the keyboard: not an --frames run nor an agent (--control).
bool interactive(const Options& o) { return o.frames == 0 && o.control_port == 0; }

// How far into the next fixed step to draw (smooth motion at any refresh rate).
// Exact state when stepping frame by frame (--frames, dev tools pause).
float blend_for(const Options& o, const alpha::core::FixedStep& clock, const Tools& tools, const Control& control) {
  if (o.frames > 0 || tools.paused() || control.paused()) return 1.0f;
  return static_cast<float>(clock.fraction());
}

// Game frame, then dev overlays drawn with it (physics view), then the dev tools windows.
void draw(alpha::script::Host& host, alpha::render::Renderer& renderer, Tools& tools, FrameStats stats,
          float blend) {
  alpha::render::Frame frame = host.frame(blend);
  tools.decorate(frame, host.world());
  alpha::core::timed(host.frame_times()[alpha::core::Section::render], [&] { renderer.draw(frame); });
  host.draw_ui(renderer.sdl(), static_cast<double>(SDL_GetTicksNS()) / 1e9);
  host.end_frame();  // the dev tools below show this frame's times
  stats.drawn = renderer.stats();
  tools.draw(host, stats);
}

// Each new pausing script error, once, on stderr (a reload may fix one and bring another).
void report_error(const alpha::script::Host& host, std::string& reported) {
  if (!host.paused() || host.error() == reported) return;
  reported = host.error();
  std::fprintf(stderr, "%s\n", reported.c_str());
}

// --frames runs (dev/release): what the last frame drew; --perf PATH: the bench report.
void report_render([[maybe_unused]] const Options& o, [[maybe_unused]] alpha::script::Host& host,
                   [[maybe_unused]] const alpha::render::Renderer& renderer) {
#if ALPHA_DEBUG_TOOLS  // diagnostics
  if (o.frames > 0) alpha::script::report_run(o.perf, host, renderer.stats());
#endif
}

// ctx.window requests applied, real state recorded; with expand the scene sees the new size.
void sync_window(SDL_Window* window, alpha::script::Host& host, const alpha::render::Renderer& renderer) {
  alpha::platform::sync_window(window, host.window());
  host.resize_view(renderer.logical_size());
}

void run(const Options& o, alpha::script::Host& host, alpha::render::Renderer& renderer, Tools& tools, SDL_Window* window) {
  alpha::platform::AudioOut audio;
  if (!audio.ok()) SDL_Log("no audio device (%s); running silent", SDL_GetError());
  Devices d;
  alpha::core::FixedStep clock(kDt, 5);
  Uint64 last = SDL_GetTicksNS();
  std::string reported;
  bool running = true;
  // dev/release: reload when the pack changes. Not in --frames runs (deterministic) nor under an
  // agent (--control): the agent reloads explicitly, so a re-export never restarts a scene mid-step.
  alpha::script::HotReload hot_reload(host, interactive(o));
  Control control = make_control(o, host, renderer, [&d] { return alpha::platform::consume(d.input, d.pads.slots()); });
  alpha::script::AppEvents app(interactive(o));  // agents and --frames own time: focus never pauses
  for (int frame = 0; running && pump(d, tools, host, renderer.sdl(), app) && (o.frames == 0 || frame < o.frames); ++frame) {
    tap(o, frame, d.input);
    app.configure(host);
    const double elapsed = seconds_since(last);
    const int steps = app.steps(clock, control.steps(steps_for_frame(o, clock, tools, elapsed)));
    for (int s = 0; s < steps; ++s) control.step();
    d.pads.rumble(host.take_rumble());
    sync_window(window, host, renderer);
    audio.pause(app.paused());
    alpha::core::timed(host.frame_times()[alpha::core::Section::audio], [&] { audio.pump(host.mixer()); });
    draw(host, renderer, tools, FrameStats{elapsed * 1000.0, steps, {}}, blend_for(o, clock, tools, control));
    hot_reload.poll(host, static_cast<double>(SDL_GetTicksNS()) / 1e9);
    report_error(host, reported);
    if (!o.screenshot.empty() && frame + 1 == o.frames) renderer.save_bmp(o.screenshot);
    running = control.poll(elapsed * 1000.0);  // agent requests see this frame drawn, before present
    renderer.present();
    app.idle();
  }
  report_render(o, host, renderer);
}


// No game argument and no fused pack: tell the developer (usage) or the player.
int no_game() {
#if ALPHA_DEBUG_TOOLS
  std::fprintf(stderr,
               "usage: alpha_star <pack-dir | game.js> [--frames N] [--screenshot out.bmp] [--tap INPUT] "
               "[--tap-every K] [--devtools]\n       alpha_star --test <tests.js> [--assets DIR]\n");
#else
  const char* text = "No game found. Put this program next to the game's game.jsbc (alpha export --standalone).";
  SDL_Log("%s", text);
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "alpha-star", text, nullptr);
#endif
  return 2;
}

// VSync (config.window.vsync) with the shortest frame queue the driver allows (input latency).
void limit_latency(SDL_Renderer* sdl, bool vsync) {
  SDL_SetRenderVSync(sdl, vsync ? 1 : SDL_RENDERER_VSYNC_DISABLED);
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
  if (const int code = alpha::script::run_command(options); code >= 0) return code;
#endif
  const std::string game = options.game.empty() ? alpha::platform::bundled_pack() : options.game;
  if (game.empty()) return no_game();
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) return fail("SDL_Init");
  alpha::script::Host host(save_path_for);
  const alpha::script::GameInfo info = host.load(game);
#if ALPHA_SHIPPING && !defined(__ANDROID__)  // no console: logs go to <pref dir>/log.txt (Android: logcat)
  alpha::platform::redirect_logs(alpha::platform::pref_file(info.title, "log.txt"), host.log());
#endif
  SDL_Log("alpha-star %s", alpha::core::version().data());
  SDL_Window* window = nullptr;
  SDL_Renderer* sdl = nullptr;
  alpha::platform::prepare_mobile(info.width, info.height);
  const alpha::core::Vec2 size{static_cast<float>(info.width), static_cast<float>(info.height)};
  if (!alpha::platform::open_window({info.title, size, info.window, options.hidden}, &window, &sdl)) return fail("SDL_CreateWindowAndRenderer");
  limit_latency(sdl, info.window.vsync);
  alpha::render::Renderer renderer(sdl, info.width, info.height, info.window.scaling);
  {
    Tools tools(window, sdl);  // gone before its renderer
    if (options.devtools) tools.toggle();
    run(options, host, renderer, tools, window);
  }
  host.release_ui();
  host.mixer().stop_all();  // streamed music holds SDL audio streams: released before SDL_Quit
  SDL_DestroyRenderer(sdl);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
