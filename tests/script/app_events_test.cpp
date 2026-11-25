#include <doctest/doctest.h>

#include <filesystem>
#include <string>

#include "host_fixture.hpp"
#include "platform/lifecycle.hpp"
#include "script/app_events.hpp"

using alpha::script::AppEvents;
using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

SDL_Event event_of(Uint32 type) {
  SDL_Event e{};
  e.type = type;
  return e;
}

// Logs every hook call as "pause:<ctx.app.paused>" etc.; `config` appended to its config.
std::string hooks_game(const std::string& config) {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { enter(ctx) {\n"
         "  ctx.onPause((c) => console.log('pause:' + c.app.paused));\n"
         "  ctx.onResume((c) => console.log('resume:' + c.app.paused));\n"
         "  ctx.onLowMemory(() => console.log('low'));\n"
         "} });\n"
         "export default game({ config: { title: 't', size: [100, 100]" + config + " }, start: 'play', scenes: { play } });\n";
}

std::string last_log(const Host& host) { return host.log().at(host.log().size() - 1).text; }

}  // namespace

TEST_CASE("SDL events map to app events; others mean nothing") {
  using alpha::core::AppEvent;
  CHECK(alpha::platform::app_event(event_of(SDL_EVENT_DID_ENTER_BACKGROUND)) == AppEvent::background);
  CHECK(alpha::platform::app_event(event_of(SDL_EVENT_WILL_ENTER_FOREGROUND)) == AppEvent::foreground);
  CHECK(alpha::platform::app_event(event_of(SDL_EVENT_WINDOW_MAXIMIZED)) == AppEvent::restored);
  CHECK(alpha::platform::app_event(event_of(SDL_EVENT_LOW_MEMORY)) == AppEvent::low_memory);
  CHECK_FALSE(alpha::platform::app_event(event_of(SDL_EVENT_KEY_DOWN)).has_value());
}

TEST_CASE("focus lost pauses the game: hooks run, ctx.app.paused follows, resume is reported once") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", hooks_game("")));
  REQUIRE(host.error().empty());
  AppEvents app(true);
  app.configure(host);
  CHECK_FALSE(app.handle(event_of(SDL_EVENT_KEY_DOWN), host));
  CHECK(app.handle(event_of(SDL_EVENT_WINDOW_FOCUS_LOST), host));
  CHECK(app.paused());
  CHECK(host.app_paused());
  CHECK(last_log(host) == "pause:true");
  alpha::core::FixedStep clock(0.25, 8);
  clock.advance(0.2);
  CHECK(app.steps(clock, 3) == 0);  // paused: no steps
  app.idle();                       // waits briefly (no busy loop)
  app.handle(event_of(SDL_EVENT_WINDOW_FOCUS_GAINED), host);
  CHECK(last_log(host) == "resume:false");
  CHECK(app.steps(clock, 2) == 2);
  CHECK(clock.fraction() == 0.0);  // resumed: the clock restarted
  clock.advance(0.2);
  app.steps(clock, 1);
  CHECK(clock.fraction() > 0.0);   // reset only once per resume
  app.idle();                       // running: returns at once
  app.handle(event_of(SDL_EVENT_LOW_MEMORY), host);
  CHECK(last_log(host) == "low");
}

TEST_CASE("pauseOnFocusLoss false (or an agent run) ignores focus; the background still pauses") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", hooks_game(", pauseOnFocusLoss: false")));
  AppEvents app(true);
  app.configure(host);
  app.handle(event_of(SDL_EVENT_WINDOW_MINIMIZED), host);
  CHECK_FALSE(app.paused());
  app.handle(event_of(SDL_EVENT_WILL_ENTER_BACKGROUND), host);
  CHECK(app.paused());
  AppEvents agent(false);
  agent.configure(host);
  agent.handle(event_of(SDL_EVENT_WINDOW_FOCUS_LOST), host);
  CHECK_FALSE(agent.paused());
}

TEST_CASE("bad pauseOnFocusLoss and bad hooks are errors that name the fix") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", hooks_game(", pauseOnFocusLoss: 'yes'")));
  CHECK(host.error().find("config.pauseOnFocusLoss must be true") != std::string::npos);
  Host other(box.save());
  other.load(box.write("h.js", alpha::test::tiny("enter", "ctx.onPause(3);")));
  CHECK(other.error().find("onPause(fn): fn must be a function") != std::string::npos);
}

TEST_CASE("a throwing onPause hook pauses the game with the error, it never crashes") {
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", alpha::test::tiny("enter", "ctx.onPause(() => { throw new Error('hook broke'); });")));
  REQUIRE(host.error().empty());
  host.set_app_paused(true);
  CHECK(host.error().find("hook broke") != std::string::npos);
  host.set_app_paused(false);  // a paused game ignores app news
}

TEST_CASE("simulate appPauses: no steps while paused, hooks at both edges") {
  Sandbox box;
  const std::string game = std::filesystem::path(box.write(
      "game.js",
      "import { define } from 'alpha'; const { scene, game } = define();\n"
      "const play = scene('play', { state: () => ({ steps: 0, log: [] }),\n"
      "  enter(ctx) { ctx.onPause(() => ctx.state.log.push('p' + ctx.state.steps));"
      " ctx.onResume(() => ctx.state.log.push('r' + ctx.state.steps)); },\n"
      "  update(ctx) { ctx.state.steps++; } });\n"
      "export default game({ config: { title: 't', size: [100, 100] }, start: 'play', scenes: { play } });\n"))
      .generic_string();
  Host host(box.save());
  const auto results = host.test(box.write(
      "tests.js", "import { simulate } from 'alpha/test';\nimport game from '" + game + "';\n"
                  "export function t() { const r = simulate(game, { seconds: 1, appPauses: [[10, 30]] });\n"
                  "  if (JSON.stringify(r.state) !== JSON.stringify({ steps: 40, log: ['p10', 'r10'] })) throw new Error(JSON.stringify(r.state)); }\n"
                  "export function bad() { simulate(game, { seconds: 1, appPauses: [[5, 2]] }); }\n"));
  REQUIRE(results.size() == 2);
  CHECK(results[0].error.find("appPauses must be") != std::string::npos);  // "bad" sorts first
  CHECK(results[1].passed());
}

// SDL 3.2 never queues the OS lifecycle events (background, foreground, terminating,
// low memory): it only calls event watchers. Pushing then flushing the queue is that.
static void os_says(Uint32 type) {
  SDL_Event e = event_of(type);
  REQUIRE(SDL_PushEvent(&e));
  SDL_FlushEvent(type);
}

TEST_CASE("the OS announcing background pauses at once; the loop drains it, then foreground resumes") {
  REQUIRE(SDL_InitSubSystem(SDL_INIT_EVENTS));
  Sandbox box;
  Host host(box.save());
  host.load(box.write("g.js", hooks_game("")));
  AppEvents app(true);
  app.configure(host);
  os_says(SDL_EVENT_WILL_ENTER_BACKGROUND);
  os_says(SDL_EVENT_DID_ENTER_BACKGROUND);
  CHECK(app.paused());  // the watch saw it on the pushing thread
  alpha::core::FixedStep clock(0.25, 8);
  CHECK(app.steps(clock, 2) == 0);
  CHECK_FALSE(host.app_paused());  // no JS ran from the watch
  app.drain(host);
  CHECK(host.app_paused());  // the loop ran the hooks
  CHECK(last_log(host) == "pause:true");
  os_says(SDL_EVENT_WILL_ENTER_FOREGROUND);
  os_says(SDL_EVENT_DID_ENTER_FOREGROUND);
  app.drain(host);
  CHECK_FALSE(app.paused());  // the game runs again (it froze here on Android)
  CHECK(last_log(host) == "resume:false");
  os_says(SDL_EVENT_LOW_MEMORY);
  app.drain(host);
  CHECK(last_log(host) == "low");
  SDL_QuitSubSystem(SDL_INIT_EVENTS);
}
