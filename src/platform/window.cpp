#include "platform/window.hpp"

namespace alpha::platform {

core::PixelGrid output_grid(SDL_Renderer* renderer) {
  int w = 0;
  int h = 0;
  SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
  SDL_FRect shown{};
  if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, &mode) || mode == SDL_LOGICAL_PRESENTATION_DISABLED) return {};
  if (!SDL_GetRenderLogicalPresentationRect(renderer, &shown)) return {};
  return core::pixel_grid({shown.x, shown.y}, {shown.w, shown.h}, {static_cast<float>(w), static_cast<float>(h)});
}

SDL_Renderer* vulkan_renderer(SDL_Window* window) {
  const SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window);
  SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, SDL_GPU_RENDERER);
  SDL_SetStringProperty(props, SDL_PROP_GPU_DEVICE_CREATE_NAME_STRING, "vulkan");
  SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_SPIRV_BOOLEAN, true);
  SDL_Renderer* r = SDL_CreateRendererWithProperties(props);
  SDL_DestroyProperties(props);
  return r;
}

namespace {

// The Vulkan GPU renderer; SDL's default renderer (custom shader effects then
// skip with a warning) without Vulkan.
SDL_Renderer* gpu_renderer(SDL_Window* window) {
  if (SDL_Renderer* r = vulkan_renderer(window)) return r;
  SDL_Log("assets.shaders: no Vulkan GPU renderer (%s); custom shader effects are skipped", SDL_GetError());
  return SDL_CreateRenderer(window, nullptr);
}

// ctx.safeArea: SDL's safe area (window points) as output pixels, measured
// against the game view of the logical presentation.
core::Insets safe_insets(SDL_Window* window) {
  SDL_Renderer* renderer = SDL_GetRenderer(window);
  SDL_Rect safe{};
  int w = 0;
  int h = 0;
  if (!renderer || !SDL_GetWindowSafeArea(window, &safe)) return {};
  if (!SDL_GetRenderLogicalPresentation(renderer, &w, &h, nullptr) || w <= 0 || h <= 0) return {};
  const float d = SDL_GetWindowPixelDensity(window);
  const core::Vec2 lo{static_cast<float>(safe.x) * d, static_cast<float>(safe.y) * d};
  const core::Vec2 hi{static_cast<float>(safe.x + safe.w) * d, static_cast<float>(safe.y + safe.h) * d};
  return core::safe_insets(output_grid(renderer), {static_cast<float>(w), static_cast<float>(h)}, lo, hi);
}

// The primary display's area without taskbars, in window units; {} when unknown (headless).
core::Vec2 usable_display() {
  SDL_Rect r{};
  if (!SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &r)) return {};
  return {static_cast<float>(r.w), static_cast<float>(r.h)};
}

}  // namespace

bool open_window(const WindowSetup& setup, SDL_Window** window, SDL_Renderer** renderer) {
  const core::WindowConfig& c = setup.config;
  SDL_WindowFlags flags = 0;
  if (c.resizable) flags |= SDL_WINDOW_RESIZABLE;
  if (core::starts_fullscreen(c, core::os_from_sdl(SDL_GetPlatform()))) flags |= SDL_WINDOW_FULLSCREEN;  // phones: immersive
  if (c.borderless) flags |= SDL_WINDOW_BORDERLESS;
  if (c.always_on_top) flags |= SDL_WINDOW_ALWAYS_ON_TOP;
  if (c.maximized) flags |= SDL_WINDOW_MAXIMIZED;
  if (setup.hidden) flags |= SDL_WINDOW_HIDDEN;
  flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;  // output in real pixels on high-DPI screens; text is made at that size
  const core::Vec2 size = core::initial_window_size(setup.game, c.stretch.scale, usable_display());
  if (!setup.gpu) {
    return SDL_CreateWindowAndRenderer(setup.title.c_str(), static_cast<int>(size.x), static_cast<int>(size.y), flags,
                                       window, renderer);
  }
  *window = SDL_CreateWindow(setup.title.c_str(), static_cast<int>(size.x), static_cast<int>(size.y), flags);
  *renderer = *window ? gpu_renderer(*window) : nullptr;
  return *renderer != nullptr;
}

bool window_event(const SDL_Event& event, const core::WindowConfig& config, core::WindowState& state) {
  if (!config.alt_enter || event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) return false;
  if (event.key.scancode != SDL_SCANCODE_RETURN || (event.key.mod & SDL_KMOD_ALT) == 0) return false;
  state.request = !state.fullscreen;
  return true;
}

void sync_window(SDL_Window* window, core::WindowState& state) {
  if (state.request) {
    SDL_SetWindowFullscreen(window, *state.request);
    SDL_SyncWindow(window);
    state.request.reset();
  }
  state.fullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
  int w = 0;
  int h = 0;
  SDL_GetWindowSizeInPixels(window, &w, &h);
  state.size = {static_cast<float>(w), static_cast<float>(h)};
  state.safe = safe_insets(window);
}

}  // namespace alpha::platform
