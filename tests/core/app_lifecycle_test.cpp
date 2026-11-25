#include <doctest/doctest.h>

#include "core/app_lifecycle.hpp"

using alpha::core::AppEffects;
using alpha::core::AppEvent;
using alpha::core::AppLifecycle;

TEST_CASE("app lifecycle: losing focus pauses once, gaining it resumes once") {
  AppLifecycle app(true);
  CHECK_FALSE(app.paused());
  const AppEffects lost = app.handle(AppEvent::focus_lost);
  CHECK(lost.pause);
  CHECK_FALSE(lost.resume);
  CHECK(app.paused());
  CHECK_FALSE(app.handle(AppEvent::focus_lost).pause);  // already paused
  const AppEffects back = app.handle(AppEvent::focus_gained);
  CHECK(back.resume);
  CHECK_FALSE(app.paused());
  CHECK_FALSE(app.handle(AppEvent::focus_gained).resume);
}

TEST_CASE("app lifecycle: without pauseOnFocusLoss focus and minimizing change nothing") {
  AppLifecycle app(false);
  CHECK_FALSE(app.handle(AppEvent::focus_lost).pause);
  CHECK_FALSE(app.handle(AppEvent::minimized).pause);
  CHECK_FALSE(app.paused());
}

TEST_CASE("app lifecycle: the background always pauses and flushes, whatever the focus rule") {
  AppLifecycle app(false);
  const AppEffects bg = app.handle(AppEvent::background);
  CHECK(bg.pause);
  CHECK(bg.flush);
  CHECK(app.paused());
  CHECK_FALSE(app.handle(AppEvent::background).pause);  // will + did enter background
  CHECK(app.handle(AppEvent::foreground).resume);
}

TEST_CASE("app lifecycle: resume waits until every reason to pause is gone") {
  AppLifecycle app(true);
  CHECK(app.handle(AppEvent::minimized).pause);
  CHECK_FALSE(app.handle(AppEvent::focus_lost).pause);
  CHECK_FALSE(app.handle(AppEvent::restored).resume);  // still unfocused
  CHECK(app.paused());
  CHECK(app.handle(AppEvent::focus_gained).resume);
}

TEST_CASE("app lifecycle: terminating pauses for good and flushes; low memory only reports") {
  AppLifecycle app(true);
  const AppEffects low = app.handle(AppEvent::low_memory);
  CHECK(low.low_memory);
  CHECK_FALSE(low.pause);
  CHECK_FALSE(low.flush);
  const AppEffects end = app.handle(AppEvent::terminating);
  CHECK(end.pause);
  CHECK(end.flush);
  CHECK_FALSE(app.handle(AppEvent::foreground).resume);
  CHECK(app.paused());
}

TEST_CASE("app lifecycle: changing the focus rule while unfocused pauses or resumes at once") {
  AppLifecycle app(false);
  app.handle(AppEvent::focus_lost);
  CHECK(app.pause_on_focus_loss(true).pause);
  CHECK(app.pause_on_focus_loss(false).resume);
  CHECK_FALSE(app.paused());
}
