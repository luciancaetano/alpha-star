#include "mcp/game_session.hpp"

#include <SDL3_image/SDL_image.h>

#include <algorithm>
#include <numeric>

#include "core/error.hpp"
#include "platform/vfs.hpp"

namespace alpha::mcp {

namespace {

// PNG bytes of what the renderer has drawn so far this frame; empty on failure.
std::string read_png(SDL_Renderer* sdl) {
  SDL_Surface* pixels = SDL_RenderReadPixels(sdl, nullptr);
  auto png = platform::vfs::written([pixels](SDL_IOStream* io) { return pixels && IMG_SavePNG_IO(pixels, io, false); });
  SDL_DestroySurface(pixels);
  return png.value_or("");
}

}  // namespace

GameSession::GameSession(script::Host& host, render::Renderer& renderer, DeviceInput devices, float dt)
    : host_(host), renderer_(renderer), devices_(std::move(devices)), dt_(dt) {}

void GameSession::step_once() {
  const bool runs = !host_.paused();
  host_.step(agent_.apply(devices_()), dt_);
  frame_ += runs ? 1U : 0U;
}

void GameSession::frame_done(double frame_ms) {
  if (frame_ms > 0.0) frame_ms_[frames_seen_++ % frame_ms_.size()] = frame_ms;
}

// {"frames", "total", <section>: {p50, p95, max} ms, "jsBytes", "jsObjects"}
std::string GameSession::perf_summary() const {
  std::string out = core::to_json(host_.perf());
  const script::JsMemory js = host_.js_memory();
  out.pop_back();  // the closing brace: the heap joins the same object
  return out + ",\"jsBytes\":" + std::to_string(static_cast<long long>(js.used_bytes)) +
         ",\"jsObjects\":" + std::to_string(static_cast<long long>(js.objects)) + "}";
}

double GameSession::fps() const {
  const std::size_t n = std::min(frames_seen_, frame_ms_.size());
  const double total = std::accumulate(frame_ms_.begin(), frame_ms_.begin() + static_cast<std::ptrdiff_t>(n), 0.0);
  return total > 0.0 ? 1000.0 * static_cast<double>(n) / total : 0.0;
}

GameState GameSession::state() {
  GameState s;
  s.scene = host_.scene();
  s.paused = paused_;
  s.error = host_.error();
  s.frame = frame_;
  s.fps = fps();
  if (const ecs::World* world = host_.world()) {
    s.entities = ecs::list_entities(*world, host_.tags(), "");
    s.joints = world->physics().joints().count();
  }
  if (const ui::Ui* game_ui = host_.ui()) s.ui = game_ui->view();
  s.locale = host_.locale();
  s.music = host_.mixer().music_name();
  s.app_paused = host_.app_paused();
  s.perf = perf_summary();
  return s;
}

void GameSession::step(int frames) {
  if (!paused_) throw core::Error("step needs a paused game: send pause first (resume runs in real time again)");
  for (int i = 0; i < frames; ++i) step_once();
}

void GameSession::action(std::string_view name, Press press, std::size_t player) {
  const core::Actions& actions = host_.controls().actions;
  const auto id = actions.find(name);
  if (!id) throw core::Error("unknown action \"" + std::string(name) + "\"; config.actions declares: " + actions.names());
  const core::InputSet& inputs = actions.inputs(*id);
  if (press == Press::Tap) agent_.tap(player, inputs);
  else if (press == Press::Hold) agent_.hold(player, inputs);
  else agent_.release(player, inputs);
}

void GameSession::axis(std::string_view name, std::size_t player, std::optional<float> value) {
  core::Axes& axes = host_.controls().axes;
  const auto id = axes.find(name);
  if (!id) throw core::Error("unknown axis \"" + std::string(name) + "\"; config.axes declares: " + axes.names());
  axes.hold(*id, player, value);
}

void GameSession::click(std::string_view doc, std::string_view id) {
  game_ui().click(std::string(doc), std::string(id));
}

ui::Ui& GameSession::game_ui() {
  ui::Ui* game_ui = host_.ui();
  if (!game_ui) throw core::Error("the game has no UI (it did not boot: see state.error)");
  return *game_ui;
}

void GameSession::reload() {
#if ALPHA_DEBUG_TOOLS
  const std::string error = host_.reload();
  if (!error.empty()) throw core::Error(error);
#else
  throw core::Error("reload needs an engine built with ALPHA_DEBUG_TOOLS (dev or release)");
#endif
}

Png GameSession::screenshot() {
  renderer_.draw(host_.frame());
  host_.draw_ui(renderer_.sdl(), static_cast<double>(SDL_GetTicksNS()) / 1e9);
  Png png{read_png(renderer_.sdl()), 0, 0};
  if (png.bytes.empty()) throw core::Error(std::string("screenshot failed: ") + SDL_GetError());
  SDL_GetCurrentRenderOutputSize(renderer_.sdl(), &png.width, &png.height);
  return png;
}

}  // namespace alpha::mcp
