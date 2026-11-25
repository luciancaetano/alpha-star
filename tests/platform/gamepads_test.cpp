#include <doctest/doctest.h>

#include "platform/gamepads.hpp"

using namespace alpha::platform;

namespace {

SDL_Event device(Uint32 type, SDL_JoystickID which) {
  SDL_Event e{};
  e.type = type;
  e.gdevice.which = which;
  return e;
}

}  // namespace

TEST_CASE("Gamepads ignore devices SDL cannot open and rumble nobody without pads") {
  Gamepads pads;
  CHECK_FALSE(pads.handle(device(SDL_EVENT_GAMEPAD_ADDED, 12345)));
  CHECK_FALSE(pads.slots().slot_of(12345));
  CHECK(pads.slots().players() == 1);
  CHECK_FALSE(pads.handle(device(SDL_EVENT_GAMEPAD_REMOVED, 12345)));
  alpha::core::RumbleRequests all;
  all.fill(alpha::core::Rumble{1.0f, 0.5f});
  pads.rumble(all);  // headless: nothing to vibrate, nothing breaks
  SDL_Event quit{};
  quit.type = SDL_EVENT_QUIT;
  CHECK_FALSE(pads.handle(quit));
}

namespace {

struct Rumbled {
  Uint16 low = 0;
  int calls = 0;
};

bool SDLCALL on_rumble(void* userdata, Uint16 low, Uint16) {
  auto* r = static_cast<Rumbled*>(userdata);
  r->low = low;
  ++r->calls;
  return true;
}

SDL_JoystickID virtual_device(SDL_JoystickType type, int axes, int buttons, Rumbled* rumbled) {
  SDL_VirtualJoystickDesc desc;
  SDL_INIT_INTERFACE(&desc);
  desc.type = static_cast<Uint16>(type);
  desc.naxes = static_cast<Uint16>(axes);
  desc.nbuttons = static_cast<Uint16>(buttons);
  desc.name = "alpha virtual device";
  desc.userdata = rumbled;
  desc.Rumble = on_rumble;
  return SDL_AttachVirtualJoystick(&desc);
}

}  // namespace

TEST_CASE("a virtual gamepad gets player 1, rumbles, and frees its slot when removed") {
  REQUIRE(SDL_InitSubSystem(SDL_INIT_GAMEPAD));
  Rumbled rumbled;
  {
    Gamepads pads;
    const SDL_JoystickID id = virtual_device(SDL_JOYSTICK_TYPE_GAMEPAD, 6, 15, &rumbled);
    REQUIRE(id != 0);
    REQUIRE(SDL_IsGamepad(id));
    CHECK_FALSE(pads.handle(device(SDL_EVENT_GAMEPAD_ADDED, id)));
    CHECK(pads.slots().slot_of(id) == 0u);
    alpha::core::RumbleRequests req;
    req[0] = alpha::core::Rumble{0.5f, 0.2f};
    pads.rumble(req);
    CHECK(rumbled.calls == 1);
    CHECK(rumbled.low == 32767);
    CHECK(pads.handle(device(SDL_EVENT_GAMEPAD_REMOVED, id)) == 0u);
    CHECK_FALSE(pads.slots().slot_of(id));
    SDL_DetachVirtualJoystick(id);
  }
  SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

TEST_CASE("a joystick without a gamepad mapping is reported once and gets no player") {
  REQUIRE(SDL_InitSubSystem(SDL_INIT_GAMEPAD));
  {
    Gamepads pads;
    const SDL_JoystickID id = virtual_device(SDL_JOYSTICK_TYPE_UNKNOWN, 1, 1, nullptr);
    REQUIRE(id != 0);
    REQUIRE_FALSE(SDL_IsGamepad(id));
    SDL_Event added{};
    added.type = SDL_EVENT_JOYSTICK_ADDED;
    added.jdevice.which = id;
    CHECK_FALSE(pads.handle(added));
    CHECK_FALSE(pads.handle(added));
    CHECK(pads.unmapped_reported() == (ALPHA_DEBUG_TOOLS ? 1u : 0u));
    CHECK(pads.slots().players() == 1);
    SDL_DetachVirtualJoystick(id);
  }
  SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}
