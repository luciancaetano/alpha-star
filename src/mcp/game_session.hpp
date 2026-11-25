#pragma once

#include <array>
#include <cstdint>
#include <functional>

#include "core/input.hpp"
#include "mcp/agent_input.hpp"
#include "mcp/session.hpp"
#include "render/renderer.hpp"
#include "script/host.hpp"

namespace alpha::mcp {

// The agent's Session over the real game. Lives on the main thread; the main
// loop runs every fixed step through step_once() so agent input and the frame
// count see each step exactly once.
class GameSession final : public Session {
 public:
  using DeviceInput = std::function<core::InputFrame()>;
  GameSession(script::Host& host, render::Renderer& renderer, DeviceInput devices, float dt);

  // Main loop side.
  void step_once();
  bool paused() const { return paused_; }
  bool quit_requested() const { return quit_; }
  // Real time of the last rendered frame (for fps).
  void frame_done(double frame_ms);

  // Session.
  GameState state() override;
  void set_paused(bool paused) override { paused_ = paused; }
  void step(int frames) override;
  void action(std::string_view name, Press press, std::size_t player) override;
  void axis(std::string_view name, std::size_t player, std::optional<float> value) override;
  void click(std::string_view doc, std::string_view id) override;
  Png screenshot() override;
  const core::LogRing& log() const override { return host_.log(); }
  void quit() override { quit_ = true; }
  void reload() override;

 private:
  ui::Ui& game_ui();
  double fps() const;

  script::Host& host_;
  render::Renderer& renderer_;
  DeviceInput devices_;
  float dt_;
  AgentInput agent_;
  bool paused_ = false;
  bool quit_ = false;
  std::uint64_t frame_ = 0;
  // fps = frames / time over the last frames (a startup hiccup fades out fast).
  std::array<double, 30> frame_ms_{};
  std::size_t frames_seen_ = 0;
};

}  // namespace alpha::mcp
