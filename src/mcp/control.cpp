#include "mcp/control.hpp"

#include <SDL3/SDL.h>

namespace alpha::mcp {

namespace {

constexpr std::size_t kMaxLine = 1 << 20;            // a request is a few hundred bytes
constexpr std::uint64_t kHiddenFrameNs = 16'666'667;  // ~60 fps without vsync

}  // namespace

ControlConfig control_config(const core::CliOptions& options) {
  const char* token = SDL_getenv("ALPHA_CONTROL_TOKEN");
  return {options.control_port > 0, options.control_port, options.hidden, token ? token : ""};
}

Control::Control(const ControlConfig& config, script::Host& host, render::Renderer& renderer,
                 GameSession::DeviceInput devices, float dt)
    : config_(config), session_(host, renderer, std::move(devices), dt), dispatcher_(session_, config.token) {
  if (!config.listen) return;
  try {
    server_ = Socket::listen(config.port);
    // Agents (alpha mcp) wait for this line; it is the only way to learn a port chosen by the system.
    SDL_Log("%s: listening on 127.0.0.1:%d", kProtocol.data(), server_.port());
  } catch (const std::exception& e) {
    SDL_Log("%s", e.what());
  }
}

bool Control::poll(double frame_ms) {
  session_.frame_done(frame_ms);
  if (server_.valid()) serve();
  pace();
  return !session_.quit_requested();
}

// The current client first, so one that just closed frees the slot for a waiting one.
void Control::serve() {
  if (client_.valid()) {
    const bool open = client_.receive(inbox_);
    answer_lines();
    if (!open) drop();
  }
  accept_clients();
}

// One client at a time: a second one is told why and closed.
void Control::accept_clients() {
  for (Socket s = server_.accept(); s.valid(); s = server_.accept()) {
    if (client_.valid()) {
      s.send(json::object({{"ok", "false"}, {"error", json::str("busy: another client is connected")}}) + "\n");
      continue;
    }
    client_ = std::move(s);
    inbox_.clear();
    dispatcher_.reset();
    client_.send(dispatcher_.greeting() + "\n");
  }
}

void Control::answer_lines() {
  for (auto nl = inbox_.find('\n'); client_.valid() && nl != std::string::npos; nl = inbox_.find('\n')) {
    std::string line = inbox_.substr(0, nl);
    inbox_.erase(0, nl + 1);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty()) continue;
    if (!client_.send(dispatcher_.handle(line) + "\n")) drop();
  }
  if (client_.valid() && inbox_.size() > kMaxLine) {
    client_.send(json::object({{"ok", "false"}, {"error", json::str("request line too long; closing")}}) + "\n");
    drop();
  }
}

void Control::drop() {
  client_ = Socket();
  inbox_.clear();
}

// Hidden windows get no vsync wait from most drivers: cap the loop at ~60 fps
// so an agent's game does not spin a CPU core.
void Control::pace() {
  if (!config_.hidden) return;
  const std::uint64_t now = SDL_GetTicksNS();
  if (last_ns_ != 0 && now - last_ns_ < kHiddenFrameNs) SDL_DelayNS(kHiddenFrameNs - (now - last_ns_));
  last_ns_ = SDL_GetTicksNS();
}

}  // namespace alpha::mcp
