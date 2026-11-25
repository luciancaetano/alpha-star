#include "platform/lifecycle.hpp"

#include <array>
#include <utility>

namespace alpha::platform {

namespace {

using core::AppEvent;

constexpr std::array<std::pair<Uint32, AppEvent>, 11> kEvents{{
    {SDL_EVENT_WILL_ENTER_BACKGROUND, AppEvent::background},
    {SDL_EVENT_DID_ENTER_BACKGROUND, AppEvent::background},
    {SDL_EVENT_WILL_ENTER_FOREGROUND, AppEvent::foreground},
    {SDL_EVENT_DID_ENTER_FOREGROUND, AppEvent::foreground},
    {SDL_EVENT_TERMINATING, AppEvent::terminating},
    {SDL_EVENT_LOW_MEMORY, AppEvent::low_memory},
    {SDL_EVENT_WINDOW_FOCUS_LOST, AppEvent::focus_lost},
    {SDL_EVENT_WINDOW_FOCUS_GAINED, AppEvent::focus_gained},
    {SDL_EVENT_WINDOW_MINIMIZED, AppEvent::minimized},
    {SDL_EVENT_WINDOW_RESTORED, AppEvent::restored},
    {SDL_EVENT_WINDOW_MAXIMIZED, AppEvent::restored},  // un-minimized straight to maximized
}};

}  // namespace

std::optional<core::AppEvent> app_event(const SDL_Event& event) {
  for (const auto& [type, app] : kEvents) {
    if (event.type == type) return app;
  }
  return std::nullopt;
}

}  // namespace alpha::platform
