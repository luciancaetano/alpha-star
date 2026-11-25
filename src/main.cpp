#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <string_view>

#include "core/fixed_step.hpp"
#include "core/version.hpp"
#include "platform/audio_out.hpp"
#include "platform/sdl_input.hpp"
#include "render/renderer.hpp"
#include "script/host.hpp"

// Composition root: alpha_star <pack-dir | game.js> [options]  (pack = `alpha export` output)
// With no arguments it runs the pack it sits in (standalone export).
// alpha_star --test <tests.js>: runs every exported test function headlessly
// (no window, no audio), one PASS/FAIL line each; exit code 0 iff all pass.
//   --frames N        run exactly N frames, one fixed step each (deterministic)
//   --screenshot PATH save the last frame as BMP (with --frames)
//   --tap INPUT       input name to tap headlessly, e.g. Space
//   --tap-every K     tap it every K frames
namespace {

constexpr double kDt = 1.0 / 60.0;
constexpr float kWindowScale = 1.5f;

struct Options {
  std::string game;
  int frames = 0;  // 0 = run until closed
  std::string screenshot;
  std::string tap;
  int tap_every = 0;
};

// Standalone pack (`alpha export --standalone`): main.js next to the executable.
std::string fused_pack() {
  const char* base = SDL_GetBasePath();
  if (!base || !std::filesystem::exists(std::filesystem::path(base) / "main.js")) return "";
  return base;
}

Options parse(int argc, char** argv) {
  Options o;
  const bool has_game = argc > 1 && argv[1][0] != '-';
  o.game = has_game ? argv[1] : fused_pack();
  for (int i = has_game ? 2 : 1; i + 1 < argc; i += 2) {
    const std::string_view flag = argv[i];
    if (flag == "--frames") o.frames = std::stoi(argv[i + 1]);
    if (flag == "--screenshot") o.screenshot = argv[i + 1];
    if (flag == "--tap") o.tap = argv[i + 1];
    if (flag == "--tap-every") o.tap_every = std::stoi(argv[i + 1]);
  }
  return o;
}

// ctx.save lives in the per-user data dir, never next to the game (which may be
// read-only or shared): e.g. %APPDATA%/alpha-star/<title>/save.json.
std::string save_path_for(const std::string& title) {
  char* dir = SDL_GetPrefPath("alpha-star", title.c_str());
  if (!dir) {
    SDL_Log("no per-user data dir (%s); saves disabled", SDL_GetError());
    return "";
  }
  std::string path = std::string(dir) + "save.json";
  SDL_free(dir);
  return path;
}

int fail(const char* what) {
  SDL_Log("%s failed: %s", what, SDL_GetError());
  SDL_Quit();
  return 1;
}

bool pump(alpha::platform::InputState& input) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) return false;
    if (event.type == SDL_EVENT_GAMEPAD_ADDED) SDL_OpenGamepad(event.gdevice.which);
    alpha::platform::apply_event(input, event);
  }
  return true;
}

void tap(const Options& o, int frame, alpha::platform::InputState& input) {
  const auto code = alpha::core::parse_input(o.tap);
  if (code && o.tap_every > 0 && frame % o.tap_every == 0) input.pressed.set(*code);
}

// Steps to run this frame: exactly 1 in --frames mode, real time otherwise.
int steps_for_frame(const Options& o, alpha::core::FixedStep& clock, Uint64& last) {
  const Uint64 now = SDL_GetTicksNS();
  const double elapsed = static_cast<double>(now - last) / 1e9;
  last = now;
  return o.frames > 0 ? 1 : clock.advance(elapsed);
}

void run(const Options& o, alpha::script::Host& host, alpha::render::Renderer& renderer) {
  alpha::platform::AudioOut audio;
  if (!audio.ok()) SDL_Log("no audio device (%s); running silent", SDL_GetError());
  alpha::platform::InputState input;
  alpha::core::FixedStep clock(kDt, 5);
  Uint64 last = SDL_GetTicksNS();
  bool reported = false;
  for (int frame = 0; pump(input) && (o.frames == 0 || frame < o.frames); ++frame) {
    tap(o, frame, input);
    const int steps = steps_for_frame(o, clock, last);
    for (int s = 0; s < steps; ++s) host.step(alpha::platform::consume(input), static_cast<float>(kDt));
    audio.pump(host.mixer());
    renderer.draw(host.frame());
    if (host.paused() && !reported) reported = std::fprintf(stderr, "%s\n", host.error().c_str()) > 0;
    if (!o.screenshot.empty() && frame + 1 == o.frames) renderer.save_bmp(o.screenshot);
    renderer.present();
  }
}

// Error text indented under its FAIL line, without trailing blank lines.
std::string indented(const std::string& text) {
  std::string out;
  for (const char c : text.substr(0, text.find_last_not_of("\r\n") + 1)) {
    out += c == '\n' ? std::string("\n    ") : std::string(1, c);
  }
  return out;
}

int run_tests(const char* path) {
  alpha::script::Host host([](const std::string&) { return std::string(); });
  const auto results = host.test(path);
  std::size_t failed = 0;
  for (const auto& r : results) {
    failed += r.passed() ? 0U : 1U;
    if (r.passed()) std::printf("PASS %s\n", r.name.c_str());
    else std::printf("FAIL %s\n    %s\n", r.name.c_str(), indented(r.error).c_str());
  }
  if (results.empty()) std::printf("no tests: %s exports no functions\n", path);
  std::printf("%zu passed, %zu failed\n", results.size() - failed, failed);
  return failed == 0 && !results.empty() ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 3 && std::string_view(argv[1]) == "--test") return run_tests(argv[2]);
  const Options options = parse(argc, argv);
  if (options.game.empty()) {
    std::fprintf(stderr, "usage: alpha_star <pack-dir | game.js> [--frames N] [--screenshot out.bmp]\n");
    return 2;
  }
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) return fail("SDL_Init");
  SDL_Log("alpha-star %s", alpha::core::version().data());
  alpha::script::Host host(save_path_for);
  const alpha::script::GameInfo info = host.load(options.game);
  SDL_Window* window = nullptr;
  SDL_Renderer* sdl = nullptr;
  const auto w = static_cast<int>(static_cast<float>(info.width) * kWindowScale);
  const auto h = static_cast<int>(static_cast<float>(info.height) * kWindowScale);
  if (!SDL_CreateWindowAndRenderer(info.title.c_str(), w, h, SDL_WINDOW_RESIZABLE, &window, &sdl)) {
    return fail("SDL_CreateWindowAndRenderer");
  }
  SDL_SetRenderVSync(sdl, 1);
  alpha::render::Renderer renderer(sdl, info.width, info.height);
  run(options, host, renderer);
  SDL_DestroyRenderer(sdl);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
