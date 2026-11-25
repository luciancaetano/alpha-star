#include "platform/mobile.hpp"

#include <SDL3/SDL.h>

#include <filesystem>

#include "platform/vfs.hpp"

namespace alpha::platform {

const char* orientations(int width, int height) {
  return height > width ? "Portrait PortraitUpsideDown" : "LandscapeLeft LandscapeRight";
}

void prepare_mobile(int width, int height) {
  SDL_SetHint(SDL_HINT_ORIENTATIONS, orientations(width, height));
  SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
}

std::string bundled_pack() {
#if defined(__ANDROID__)
  return "main.js";  // APK assets: main.js and assets/... at the root
#else
  const char* base = SDL_GetBasePath();
  if (!base || !vfs::exists(std::filesystem::path(base) / "main.js")) return "";
  return base;
#endif
}

}  // namespace alpha::platform
