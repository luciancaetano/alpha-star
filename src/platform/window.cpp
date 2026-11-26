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

namespace {

// SDL's GPU renderer, told we bring SPIR-V / DXIL / MSL shaders; SDL's default
// renderer (custom shader effects then skip with a warning) when there is none.
SDL_Renderer* gpu_renderer(SDL_Window* window) {
  const SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetPointerProperty(props, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, window);
  SDL_SetStringProperty(props, SDL_PROP_RENDERER_CREATE_NAME_STRING, SDL_GPU_RENDERER);
  SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_SPIRV_BOOLEAN, true);
  SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_DXIL_BOOLEAN, true);
  SDL_SetBooleanProperty(props, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_MSL_BOOLEAN, true);
  SDL_Renderer* r = SDL_CreateRendererWithProperties(props);
  SDL_DestroyProperties(props);
  if (r) return r;
  SDL_Log("assets.shaders: no GPU renderer (%s); custom shader effects are skipped", SDL_GetError());
  return SDL_CreateRenderer(window, nullptr);
}

}  // namespace

bool open_window(const WindowSetup& setup, SDL_Window** window, SDL_Renderer** renderer) {
  const core::WindowConfig& c = setup.config;
  SDL_WindowFlags flags = 0;
  if (c.resizable) flags |= SDL_WINDOW_RESIZABLE;
  if (c.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
  if (c.borderless) flags |= SDL_WINDOW_BORDERLESS;
  if (c.always_on_top) flags |= SDL_WINDOW_ALWAYS_ON_TOP;
  if (c.maximized) flags |= SDL_WINDOW_MAXIMIZED;
  if (setup.hidden) flags |= SDL_WINDOW_HIDDEN;
  flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;  // output in real pixels on high-DPI screens; text is made at that size
  const core::Vec2 size = core::initial_window_size(setup.game, c.scaling);
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
}

}  // namespace alpha::platform
