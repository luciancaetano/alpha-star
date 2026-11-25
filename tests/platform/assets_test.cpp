#include <doctest/doctest.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <filesystem>
#include <memory>
#include <vector>

#include "core/error.hpp"
#include "platform/assets.hpp"
#include "platform/audio_out.hpp"

using namespace alpha;
namespace fs = std::filesystem;

TEST_CASE("asset loaders name the file and the accepted formats on failure") {
  CHECK_THROWS_WITH_AS(platform::load_image("no/such.png"), doctest::Contains("PNG, JPG or BMP"),
                       core::Error);
}

TEST_CASE("load_image decodes PNG into RGBA rows") {
  SDL_Surface* s = SDL_CreateSurface(3, 2, SDL_PIXELFORMAT_RGBA32);
  SDL_FillSurfaceRect(s, nullptr, SDL_MapSurfaceRGBA(s, 10, 20, 30, 255));
  const SDL_Rect corner{2, 1, 1, 1};
  SDL_FillSurfaceRect(s, &corner, SDL_MapSurfaceRGBA(s, 200, 100, 50, 128));
  const fs::path path = fs::temp_directory_path() / "alpha_test.png";
  REQUIRE(IMG_SavePNG(s, path.string().c_str()));
  SDL_DestroySurface(s);
  const core::Image img = platform::load_image(path.string());
  CHECK(img.width == 3);
  CHECK(img.height == 2);
  REQUIRE(img.rgba.size() == 24);
  CHECK(img.rgba[0] == 10);
  CHECK(img.rgba[20] == 200);
  CHECK(img.rgba[23] == 128);
  fs::remove(path);
}

TEST_CASE("audio out tops up the device queue from the mixer, in stereo frames") {
  SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
  REQUIRE(SDL_InitSubSystem(SDL_INIT_AUDIO));
  {
    platform::AudioOut out;
    REQUIRE(out.ok());
    core::Mixer mixer;
    mixer.play(std::make_shared<const std::vector<float>>(10, 0.5f), {});
    out.pump(mixer);
    CHECK(out.queued() > 0);
    CHECK(out.queued() <= platform::AudioOut::kLatency);
    CHECK(mixer.voices() == 0);  // whole sound consumed into the queue
    out.pause(true);
    CHECK(out.paused());
    out.pause(true);  // repeated: no change
    out.pause(false);
    CHECK_FALSE(out.paused());
  }
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
