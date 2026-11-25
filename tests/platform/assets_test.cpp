#include <doctest/doctest.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>

#include "core/error.hpp"
#include "platform/assets.hpp"
#include "platform/audio_out.hpp"

using namespace alpha;
namespace fs = std::filesystem;

namespace {

template <typename T>
void put(std::ofstream& f, T v) {
  f.write(reinterpret_cast<const char*>(&v), sizeof(T));
}

// 16-bit PCM WAV with `frames` frames of `channels` channels, all = value.
fs::path write_wav(const char* name, int rate, std::uint16_t channels, std::int16_t value, int frames) {
  const fs::path path = fs::temp_directory_path() / name;
  std::ofstream f(path, std::ios::binary);
  const auto data = static_cast<std::uint32_t>(frames * channels * 2);
  f.write("RIFF", 4);
  put<std::uint32_t>(f, 36 + data);
  f.write("WAVEfmt ", 8);
  put<std::uint32_t>(f, 16);
  put<std::uint16_t>(f, 1);
  put<std::uint16_t>(f, channels);
  put<std::uint32_t>(f, static_cast<std::uint32_t>(rate));
  put<std::uint32_t>(f, static_cast<std::uint32_t>(rate * channels * 2));
  put<std::uint16_t>(f, static_cast<std::uint16_t>(channels * 2));
  put<std::uint16_t>(f, 16);
  f.write("data", 4);
  put<std::uint32_t>(f, data);
  for (int i = 0; i < frames * channels; ++i) put<std::int16_t>(f, value);
  return path;
}

}  // namespace

TEST_CASE("load_wav resamples to mono 48 kHz floats") {
  const fs::path path = write_wav("alpha_test.wav", 24000, 2, 16384, 2400);
  const auto samples = platform::load_wav(path.string());
  CHECK(samples.size() == doctest::Approx(4800).epsilon(0.01));
  CHECK(samples[samples.size() / 2] == doctest::Approx(0.5f).epsilon(0.01));
  fs::remove(path);
}

TEST_CASE("asset loaders name the file and the accepted formats on failure") {
  CHECK_THROWS_WITH_AS(platform::load_wav("no/such.wav"), doctest::Contains("no/such.wav"), core::Error);
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

TEST_CASE("audio out tops up the device queue from the mixer") {
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
  }
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
