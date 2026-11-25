#pragma once

#include <cstdint>
#include <string>

#include "core/cli.hpp"
#include "mcp/dispatch.hpp"
#include "mcp/game_session.hpp"
#include "mcp/socket.hpp"

namespace alpha::mcp {

struct ControlConfig {
  bool listen = false;  // off: the game runs exactly as without ALPHA_MCP
  int port = 0;         // 0 with listen = any free port
  bool hidden = false;  // no visible window: frames are paced here instead of by vsync
  std::string token;    // non-empty: clients must say hello with it first
};

// From --control/--hidden and $ALPHA_CONTROL_TOKEN.
ControlConfig control_config(const core::CliOptions& options);

// The agent control channel as the main loop sees it: one local client at a
// time, served on the main thread between frames (never touches game state
// from anywhere else). See mcp/dispatch.hpp for the protocol.
class Control {
 public:
  Control(const ControlConfig& config, script::Host& host, render::Renderer& renderer,
          GameSession::DeviceInput devices, float dt);
  // The dispatcher holds a reference to the session: never copied or moved.
  Control(const Control&) = delete;
  Control& operator=(const Control&) = delete;
  // Steps the clock may run this frame: none while the agent paused the game.
  int steps(int wanted) const { return session_.paused() ? 0 : wanted; }
  bool paused() const { return session_.paused(); }
  // One fixed step with device input plus whatever the agent injected.
  void step() { session_.step_once(); }
  // Serves pending requests (after the frame is drawn, before present).
  // False once the agent asked to quit.
  bool poll(double frame_ms);
  int port() const { return server_.valid() ? server_.port() : 0; }

 private:
  void accept_clients();
  void serve();
  void answer_lines();
  void drop();
  void pace();

  ControlConfig config_;
  GameSession session_;
  Dispatcher dispatcher_;
  Socket server_;
  Socket client_;
  std::string inbox_;
  std::uint64_t last_ns_ = 0;
};

}  // namespace alpha::mcp
