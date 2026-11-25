#include "platform/assets.hpp"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <cstring>

#include "core/error.hpp"
#include "core/mixer.hpp"
#include "platform/vfs.hpp"

namespace alpha::platform {

namespace {

[[noreturn]] void fail(const std::string& what, const std::string& path) {
  throw core::Error(what + " \"" + path + "\": " + SDL_GetError());
}

}  // namespace

core::Image load_image(const std::string& path) {
  const auto bytes = vfs::read(path);
  if (!bytes) fail("cannot load image (PNG, JPG or BMP)", path);
  SDL_Surface* loaded = IMG_Load_IO(vfs::memory(*bytes).release(), true);
  if (!loaded) fail("cannot load image (PNG, JPG or BMP)", path);
  SDL_Surface* rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
  SDL_DestroySurface(loaded);
  if (!rgba) fail("cannot convert image", path);
  core::Image image{rgba->w, rgba->h, {}, false};
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
