#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <string>

#include "platform/mobile.hpp"

using alpha::platform::orientations;

TEST_CASE("orientation follows the game size: portrait only when taller than wide") {
  CHECK(std::string(orientations(480, 800)) == "Portrait PortraitUpsideDown");
  CHECK(std::string(orientations(800, 480)) == "LandscapeLeft LandscapeRight");
  CHECK(std::string(orientations(600, 600)) == "LandscapeLeft LandscapeRight");
}

TEST_CASE("prepare_mobile sets the orientation and traps the back button for the game") {
  alpha::platform::prepare_mobile(360, 640);
  CHECK(std::string(SDL_GetHint(SDL_HINT_ORIENTATIONS)) == "Portrait PortraitUpsideDown");
  CHECK(std::string(SDL_GetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON)) == "1");
  SDL_ResetHints();
}

TEST_CASE("no pack next to the test runner: nothing bundled") {
  CHECK(alpha::platform::bundled_pack().empty());
}
