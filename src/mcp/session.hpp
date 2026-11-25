#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/log_ring.hpp"
#include "ecs/entity_view.hpp"
#include "ui/ui.hpp"

namespace alpha::mcp {

// What an agent sees of the running game (the `state` command).
struct GameState {
  std::string scene;       // empty before the first scene starts
  bool paused = false;     // paused by the agent (pause / step)
  std::string error;       // the script error that paused the game, source-mapped; empty if none
  std::uint64_t frame = 0; // fixed steps run since start
  double fps = 0.0;
  std::vector<ecs::EntityView> entities;
  std::map<std::string, ui::DocumentView> ui;  // every document: shown or not, element texts
  int joints = 0;                              // living physics joints in the scene
  std::string locale;                          // ctx.locale; empty without assets.locales
  std::string music;                           // the music playing; empty when none
  bool app_paused = false;                     // the app is paused (focus lost, minimized, background)
  std::string perf;  // JSON: frame time spreads per section (core::to_json) + JS heap
};

// Inject an action like a device would: a one-step tap, or held until released.
enum class Press { Tap, Hold, Release };

struct Png {
  std::string bytes;
  int width = 0;
  int height = 0;
};

// The running game as the control channel drives it. Implemented over the
// real Host by GameSession; tests use fakes. Methods throw core::Error with a
// message for the agent (what failed and what is available).
class Session {
 public:
  virtual ~Session() = default;
  virtual GameState state() = 0;
  virtual void set_paused(bool paused) = 0;
  // Runs `frames` fixed steps now; only while paused.
  virtual void step(int frames) = 0;
  // player: 0-based.
  virtual void action(std::string_view name, Press press, std::size_t player) = 0;
  // nullopt releases the axis back to the devices.
  virtual void axis(std::string_view name, std::size_t player, std::optional<float> value) = 0;
  // A click on UI element `id` of document `doc`, as a player would.
  virtual void click(std::string_view doc, std::string_view id) = 0;
  // The current frame, freshly drawn (game + UI, no dev overlays).
  virtual Png screenshot() = 0;
  virtual const core::LogRing& log() const = 0;
  virtual void quit() = 0;
  // Hot reload from the game's files (script::Host::reload); throws the error
  // when the new code fails, and the previous game keeps running.
  virtual void reload() = 0;
};

}  // namespace alpha::mcp
