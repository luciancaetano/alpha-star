#include "platform/assets.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <cstring>

#include "core/error.hpp"
#include "core/mixer.hpp"

namespace alpha::platform {

namespace {

[[noreturn]] void fail(const std::string& what, const std::string& path) {
  throw core::Error(what + " \"" + path + "\": " + SDL_GetError());
}

}  // namespace

std::vector<float> load_wav(const std::string& path) {
  SDL_AudioSpec spec{};
  Uint8* data = nullptr;
  Uint32 length = 0;
  if (!SDL_LoadWAV(path.c_str(), &spec, &data, &length)) fail("cannot load sound (WAV only)", path);
  const SDL_AudioSpec mono{SDL_AUDIO_F32, 1, core::Mixer::kSampleRate};
  Uint8* converted = nullptr;
  int converted_length = 0;
  const bool ok = SDL_ConvertAudioSamples(&spec, data, static_cast<int>(length), &mono, &converted,
                                          &converted_length);
  SDL_free(data);
  if (!ok) fail("cannot convert sound", path);
  std::vector<float> samples(static_cast<std::size_t>(converted_length) / sizeof(float));
  std::memcpy(samples.data(), converted, samples.size() * sizeof(float));
  SDL_free(converted);
  return samples;
}

core::Image load_image(const std::string& path) {
  SDL_Surface* loaded = IMG_Load(path.c_str());
  if (!loaded) fail("cannot load image (PNG, JPG or BMP)", path);
  SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
  SDL_DestroySurface(loaded);
  if (!rgba) fail("cannot convert image", path);
  core::Image image{rgba->w, rgba->h, {}};
  const auto row = static_cast<std::size_t>(rgba->w) * 4;
  image.rgba.resize(row * static_cast<std::size_t>(rgba->h));
  for (int y = 0; y < rgba->h; ++y) {
    const auto* src = static_cast<const Uint8*>(rgba->pixels) + static_cast<std::ptrdiff_t>(y) * rgba->pitch;
    std::memcpy(image.rgba.data() + row * static_cast<std::size_t>(y), src, row);
  }
  SDL_DestroySurface(rgba);
  return image;
}

}  // namespace alpha::platform
