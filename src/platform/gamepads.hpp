#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <cstddef>
#include <optional>
#include <set>
#include <string>

#include "core/controls.hpp"
#include "core/pad_slots.hpp"

namespace alpha::platform {

// Open gamepads and their player slots (thin SDL layer over core::PadSlots).
// SDL3 maps generic joysticks with its built-in controller database (plus
// SDL_GAMECONTROLLERCONFIG); a joystick it cannot map is not a gamepad: it is
// reported once in the log (debug-tools builds) and ignored.
class Gamepads {
 public:
  Gamepads() = default;
  Gamepads(const Gamepads&) = delete;
  Gamepads& operator=(const Gamepads&) = delete;
  ~Gamepads();

  // Hot-plug: opens added gamepads into a slot, closes removed ones. Returns
  // the player whose gamepad went away (release its inputs), if any.
  std::optional<std::size_t> handle(const SDL_Event& event);
  const core::PadSlots& slots() const { return slots_; }
  // Starts each requested vibration; players without a gamepad are skipped.
  void rumble(const core::RumbleRequests& requests);
  // Distinct unmapped joysticks reported so far (always 0 without debug tools).
  std::size_t unmapped_reported() const { return reported_.size(); }

 private:
  void added(SDL_JoystickID id);
  std::optional<std::size_t> removed(SDL_JoystickID id);
  void report_unmapped(SDL_JoystickID id);

  core::PadSlots slots_;
  std::array<SDL_Gamepad*, core::kMaxPlayers> pads_{};
  std::set<std::string> reported_;
};

}  // namespace alpha::platform
