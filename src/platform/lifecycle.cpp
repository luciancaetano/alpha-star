#include "platform/lifecycle.hpp"

#include <array>
#include <cstdio>
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

namespace {

bool announces_leaving(std::optional<core::AppEvent> e) {
  return e == AppEvent::background || e == AppEvent::terminating;
}

// The events SDL only gives to watchers (window events are queued as usual).
bool from_os(std::optional<core::AppEvent> e) {
  return announces_leaving(e) || e == AppEvent::foreground || e == AppEvent::low_memory;
}

}  // namespace

LifecycleWatch::LifecycleWatch() { SDL_AddEventWatch(&LifecycleWatch::on_event, this); }

LifecycleWatch::~LifecycleWatch() { SDL_RemoveEventWatch(&LifecycleWatch::on_event, this); }

std::vector<core::AppEvent> LifecycleWatch::take() {
  const std::scoped_lock hold(lock_);
  leaving_.store(false);  // the caller now drives the machine with them
  return std::exchange(pending_, {});
}

bool SDLCALL LifecycleWatch::on_event(void* self, SDL_Event* event) {
  const auto app = app_event(*event);
  if (!from_os(app)) return true;
  auto* watch = static_cast<LifecycleWatch*>(self);
  const std::scoped_lock hold(watch->lock_);
  watch->pending_.push_back(*app);
  if (announces_leaving(app)) {
    watch->leaving_.store(true);
    std::fflush(stderr);
  }
  return true;
}

}  // namespace alpha::platform
