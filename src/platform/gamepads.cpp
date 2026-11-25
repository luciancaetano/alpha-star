#include "platform/gamepads.hpp"

#include <array>

namespace alpha::platform {

namespace {

std::string guid_of(SDL_JoystickID id) {
  std::array<char, 33> text{};
  SDL_GUIDToString(SDL_GetJoystickGUIDForID(id), text.data(), static_cast<int>(text.size()));
  return text.data();
}

}  // namespace

Gamepads::~Gamepads() {
  for (SDL_Gamepad* pad : pads_) {
    if (pad) SDL_CloseGamepad(pad);
  }
}

std::optional<std::size_t> Gamepads::handle(const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_GAMEPAD_ADDED:
      added(event.gdevice.which);
      return std::nullopt;
    case SDL_EVENT_GAMEPAD_REMOVED:
      return removed(event.gdevice.which);
    case SDL_EVENT_JOYSTICK_ADDED:
      if (!SDL_IsGamepad(event.jdevice.which)) report_unmapped(event.jdevice.which);
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

void Gamepads::added(SDL_JoystickID id) {
  if (slots_.slot_of(id)) return;
  SDL_Gamepad* pad = SDL_OpenGamepad(id);
  if (!pad) return;
  const auto slot = slots_.connect(id, guid_of(id));
  if (!slot) {
    SDL_CloseGamepad(pad);  // every player slot is taken
    return;
  }
  pads_[*slot] = pad;
}

std::optional<std::size_t> Gamepads::removed(SDL_JoystickID id) {
  const auto slot = slots_.disconnect(id);
  if (slot && pads_[*slot]) {
    SDL_CloseGamepad(pads_[*slot]);
    pads_[*slot] = nullptr;
  }
  return slot;
}

void Gamepads::report_unmapped(SDL_JoystickID id) {
#if ALPHA_DEBUG_TOOLS
  const std::string guid = guid_of(id);
  if (!reported_.insert(guid).second) return;
  const char* name = SDL_GetJoystickNameForID(id);
  SDL_Log("joystick \"%s\" (GUID %s) has no gamepad mapping; ignored. Add one with SDL_GAMECONTROLLERCONFIG.",
          name ? name : "?", guid.c_str());
#else
  (void)id;
#endif
}

void Gamepads::rumble(const core::RumbleRequests& requests) {
  for (std::size_t p = 0; p < core::kMaxPlayers; ++p) {
    if (!requests[p] || !pads_[p]) continue;
    const auto level = static_cast<Uint16>(requests[p]->strength * 65535.0f);
    const auto ms = static_cast<Uint32>(requests[p]->seconds * 1000.0f);
    SDL_RumbleGamepad(pads_[p], level, level, ms);
  }
}

}  // namespace alpha::platform
