#include <doctest/doctest.h>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <SDL3/SDL.h>

#include <string>

#include "host_fixture.hpp"
#include "ui/input.hpp"
#include "ui/ui.hpp"

using alpha::test::Sandbox;
using alpha::ui::Ui;

namespace {

const char* kMenu = R"(<rml><head><style>
body { width: 100%; height: 100%; }
button { display: block; width: 50px; height: 20px; tab-index: auto; }
#box { position: absolute; left: 10px; top: 10px; width: 20px; height: 20px; background-color: #ff0000; }
</style></head><body>
<div id="box"></div>
<div id="clip" style="position: absolute; left: 60px; top: 10px; width: 10px; height: 10px; overflow: hidden;"><img src="dot.bmp" style="width: 30px; height: 30px;"/></div>
<button id="play" style="margin-top: 40px;"></button>
<button id="quit"></button>
</body></rml>)";

struct Menu {
  Sandbox box;
  Ui ui{100, 100};
  Menu() {
    SDL_Surface* dot = SDL_CreateSurface(2, 2, SDL_PIXELFORMAT_RGBA32);
    SDL_FillSurfaceRect(dot, nullptr, SDL_MapSurfaceRGBA(dot, 0, 0, 255, 255));
    SDL_SaveBMP(dot, (box.dir / "dot.bmp").string().c_str());
    SDL_DestroySurface(dot);
    ui.add("menu", box.write("menu.rml", kMenu));
  }
  std::string focus() const {
    const Rml::Element* e = const_cast<Ui&>(ui).context().GetFocusElement();
    return e ? e->GetId() : "";
  }
};

SDL_Event mouse(float x, float y, bool down) {
  SDL_Event e{};
  e.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
  e.button.button = SDL_BUTTON_LEFT;
  e.button.down = down;
  e.button.x = x;
  e.button.y = y;
  return e;
}

SDL_Event pad(SDL_GamepadButton button, bool down = true) {
  SDL_Event e{};
  e.type = down ? SDL_EVENT_GAMEPAD_BUTTON_DOWN : SDL_EVENT_GAMEPAD_BUTTON_UP;
  e.gbutton.button = static_cast<Uint8>(button);
  e.gbutton.down = down;
  return e;
}

SDL_Event stick_y(float y) {
  SDL_Event e{};
  e.type = SDL_EVENT_GAMEPAD_AXIS_MOTION;
  e.gaxis.axis = SDL_GAMEPAD_AXIS_LEFTY;
  e.gaxis.value = static_cast<Sint16>(y * 32767.0f);
  return e;
}

bool tap(Ui& ui, SDL_GamepadButton button) {
  const bool consumed = alpha::ui::consumes(ui, pad(button));
  alpha::ui::consumes(ui, pad(button, false));
  return consumed;
}

}  // namespace

TEST_CASE("ui: documents load hidden; show, set and view") {
  Menu m;
  CHECK_FALSE(m.ui.view().at("menu").shown);
  m.ui.show("menu");
  m.ui.set("menu", "play", "Play <again> & more");
  const auto view = m.ui.view().at("menu");
  CHECK(view.shown);
  CHECK(view.texts.at("play") == "Play <again> & more");  // plain text, not markup
  m.ui.hide("menu");
  CHECK_FALSE(m.ui.view().at("menu").shown);
}

TEST_CASE("ui: mistakes name what exists") {
  Menu m;
  CHECK_THROWS_WITH(m.ui.show("hud"), "unknown document \"hud\"; assets.ui has: menu");
  CHECK_THROWS_WITH(m.ui.set("menu", "plya", "x"),
                    doctest::Contains("no element with id \"plya\" in document \"menu\""));
  CHECK_THROWS_WITH(m.ui.set("menu", "plya", "x"), doctest::Contains("ids: box, clip, play, quit"));
  CHECK_THROWS_WITH(m.ui.on("menu", "play", "hover"), "unknown event \"hover\"; events: click, back");
  CHECK_THROWS_WITH(m.ui.click("menu", "play"), "document \"menu\" is hidden");
  CHECK_THROWS_WITH(m.ui.add("x", (m.box.dir / "nope.rml").string()), doctest::Contains("cannot load"));
}

TEST_CASE("ui: clicks queue their handler; reset drops handlers and hides") {
  Menu m;
  m.ui.show("menu");
  const auto play = m.ui.on("menu", "play", "click");
  m.ui.click("menu", "play");
  m.ui.click("menu", "quit");  // no handler: nothing queued
  CHECK(m.ui.take_events() == std::vector<std::uint32_t>{play});
  CHECK(m.ui.take_events().empty());
  m.ui.reset();
  CHECK_FALSE(m.ui.view().at("menu").shown);
  m.ui.show("menu");
  m.ui.click("menu", "play");
  CHECK(m.ui.take_events().empty());
}

TEST_CASE("ui input: a mouse click is consumed only on an element with a handler") {
  Menu m;
  m.ui.show("menu");
  const auto play = m.ui.on("menu", "play", "click");
  CHECK(alpha::ui::consumes(m.ui, mouse(25, 50, true)));
  CHECK(alpha::ui::consumes(m.ui, mouse(25, 50, false)));
  CHECK(m.ui.take_events() == std::vector<std::uint32_t>{play});
  CHECK_FALSE(alpha::ui::consumes(m.ui, mouse(25, 70, true)));  // quit: no handler
  CHECK_FALSE(alpha::ui::consumes(m.ui, mouse(90, 90, true)));  // empty space
  m.ui.hide("menu");
  CHECK_FALSE(alpha::ui::consumes(m.ui, mouse(25, 50, true)));  // hidden documents take nothing
}

TEST_CASE("ui input: gamepad autofocus, D-pad and stick navigation, A activates, B is back") {
  Menu m;
  CHECK_FALSE(tap(m.ui, SDL_GAMEPAD_BUTTON_SOUTH));  // nothing shown: A reaches the game
  m.ui.show("menu");
  CHECK(m.focus() == "play");  // gamepad was the last device: first focusable
  const auto quit = m.ui.on("menu", "quit", "click");
  CHECK(tap(m.ui, SDL_GAMEPAD_BUTTON_DPAD_DOWN));
  CHECK(m.focus() == "quit");
  CHECK(tap(m.ui, SDL_GAMEPAD_BUTTON_SOUTH));  // consumed: must not also flap
  CHECK(m.ui.take_events() == std::vector<std::uint32_t>{quit});
  CHECK(alpha::ui::consumes(m.ui, stick_y(-0.9f)));  // up past the deadzone
  CHECK(m.focus() == "play");
  CHECK_FALSE(alpha::ui::consumes(m.ui, stick_y(-0.95f)));  // still held: no move
  CHECK_FALSE(alpha::ui::consumes(m.ui, stick_y(0.2f)));
  CHECK_FALSE(tap(m.ui, SDL_GAMEPAD_BUTTON_EAST));  // no back handler
  const auto back = m.ui.on("menu", "play", "back");
  CHECK(tap(m.ui, SDL_GAMEPAD_BUTTON_EAST));
  CHECK(m.ui.take_events() == std::vector<std::uint32_t>{back});
}

TEST_CASE("ui input: after a mouse click nothing is autofocused, so A and Space reach the game") {
  Menu m;
  alpha::ui::consumes(m.ui, mouse(90, 90, true));
  m.ui.show("menu");
  CHECK(m.focus() != "play");
  CHECK_FALSE(tap(m.ui, SDL_GAMEPAD_BUTTON_SOUTH));
}

TEST_CASE("ui: held D-pad repeats focus moves on tick") {
  Menu m;
  m.ui.used_gamepad(true);
  m.ui.show("menu");
  SDL_Event down = pad(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
  down.gbutton.timestamp = 1'000'000'000;  // 1 s
  CHECK(alpha::ui::consumes(m.ui, down));
  CHECK(m.focus() == "quit");
  m.ui.tick(1.0 + alpha::ui::PadNav::kDelay);  // wraps around to the first
  CHECK(m.focus() == "play");
}

TEST_CASE("ui renderer draws styled elements into an SDL renderer") {
  Menu m;
  m.ui.reset();  // a scene change reloads documents; they still draw
  m.ui.show("menu");
  SDL_Surface* surface = SDL_CreateSurface(100, 100, SDL_PIXELFORMAT_RGBA32);
  SDL_Renderer* sdl = SDL_CreateSoftwareRenderer(surface);
  REQUIRE(sdl);
  SDL_SetRenderDrawColor(sdl, 0, 0, 0, 255);
  SDL_RenderClear(sdl);
  m.ui.render(sdl);
  SDL_RenderPresent(sdl);
  Uint8 r = 0, g = 0, b = 0, a = 0;
  SDL_ReadSurfacePixel(surface, 20, 20, &r, &g, &b, &a);
  CHECK(r == 255);
  CHECK(g == 0);
  SDL_ReadSurfacePixel(surface, 90, 20, &r, &g, &b, &a);
  CHECK(r == 0);
  SDL_ReadSurfacePixel(surface, 65, 15, &r, &g, &b, &a);
  CHECK(b == 255);  // the image, inside its clipping box
  SDL_ReadSurfacePixel(surface, 75, 25, &r, &g, &b, &a);
  CHECK(b == 0);    // clipped (scissor)
  m.ui.attach(nullptr);
  SDL_DestroyRenderer(sdl);
  SDL_DestroySurface(surface);
}

SDL_Event key(SDL_Keycode k) {
  SDL_Event e{};
  e.type = SDL_EVENT_KEY_DOWN;
  e.key.key = k;
  e.key.down = true;
  return e;
}

TEST_CASE("ui input: keyboard Tab moves focus, Escape is back, motion and text pass through") {
  Menu m;
  m.ui.show("menu");
  CHECK(alpha::ui::consumes(m.ui, key(SDLK_TAB)));
  CHECK(m.focus() == "play");
  CHECK_FALSE(alpha::ui::consumes(m.ui, key(SDLK_ESCAPE)));  // no back handler
  m.ui.on("menu", "play", "back");
  CHECK(alpha::ui::consumes(m.ui, key(SDLK_ESCAPE)));
  SDL_Event up = key(SDLK_TAB);
  up.type = SDL_EVENT_KEY_UP;
  up.key.down = false;
  CHECK_FALSE(alpha::ui::consumes(m.ui, up));
  SDL_Event motion{};
  motion.type = SDL_EVENT_MOUSE_MOTION;
  CHECK_FALSE(alpha::ui::consumes(m.ui, motion));
  SDL_Event wheel{};
  wheel.type = SDL_EVENT_MOUSE_WHEEL;
  wheel.wheel.y = 1;
  CHECK_FALSE(alpha::ui::consumes(m.ui, wheel));  // nothing scrolls
  SDL_Event text{};
  text.type = SDL_EVENT_TEXT_INPUT;
  text.text.text = "a";
  CHECK_FALSE(alpha::ui::consumes(m.ui, text));  // no text field focused
  SDL_Event other{};
  other.type = SDL_EVENT_WINDOW_SHOWN;
  CHECK_FALSE(alpha::ui::consumes(m.ui, other));
}

TEST_CASE("ui: class=\"safe\" elements are padded by the safe area, also in documents loaded later") {
  Sandbox box;
  Ui ui{100, 100};
  const char* hud = R"(<rml><head><style>body { width: 100%; height: 100%; } #corner { display: block; width: 5px; height: 5px; }</style></head>
<body class="safe"><div id="corner"></div></body></rml>)";
  ui.add("hud", box.write("hud.rml", hud));
  ui.show("hud");
  ui.safe_area({12, 3, 0, 7});
  ui.context().Update();
  Rml::Element* corner = ui.context().GetDocument(0)->GetElementById("corner");
  CHECK(corner->GetAbsoluteOffset().x == doctest::Approx(12));
  CHECK(corner->GetAbsoluteOffset().y == doctest::Approx(3));
  ui.add("hud", box.write("hud.rml", hud));  // reloaded (hot reload): padded at once
  ui.show("hud");
  ui.context().Update();
  CHECK(ui.context().GetDocument(0)->GetElementById("corner")->GetAbsoluteOffset().x == doctest::Approx(12));
}
