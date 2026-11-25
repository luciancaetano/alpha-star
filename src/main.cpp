#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cstdio>
#include <string>
#include <string_view>

#include "core/fixed_step.hpp"
#include "core/version.hpp"
#include "platform/audio_out.hpp"
#include "platform/sdl_input.hpp"
#include "render/renderer.hpp"
#include "script/host.hpp"

// Composition root: alpha_star <game.js> [options]
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

Options parse(int argc, char** argv) {
  Options o;
  if (argc > 1) o.game = argv[1];
  for (int i = 2; i + 1 < argc; i += 2) {
    const std::string_view flag = argv[i];
    if (flag == "--frames") o.frames = std::stoi(argv[i + 1]);
    if (flag == "--screenshot") o.screenshot = argv[i + 1];
    if (flag == "--tap") o.tap = argv[i + 1];
    if (flag == "--tap-every") o.tap_every = std::stoi(argv[i + 1]);
  }
  return o;
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

}  // namespace

int main(int argc, char** argv) {
  const Options options = parse(argc, argv);
  if (options.game.empty()) {
    std::fprintf(stderr, "usage: alpha_star <game.js> [--frames N] [--screenshot out.bmp]\n");
    return 2;
  }
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) return fail("SDL_Init");
  SDL_Log("alpha-star %s", alpha::core::version().data());
  alpha::script::Host host(options.game + ".save.json");
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
