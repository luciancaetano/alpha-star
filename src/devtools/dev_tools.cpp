#include "devtools/dev_tools.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "devtools/inspector.hpp"
#include "devtools/layout.hpp"
#include "devtools/panels.hpp"
#include "devtools/profiler.hpp"
#include "devtools/routing.hpp"
#include "devtools/scale.hpp"

namespace alpha::devtools {

namespace {

// The device's first accelerometer (phones), for the shake toggle; null when there is none.
SDL_Sensor* open_accelerometer() {
  if (!SDL_InitSubSystem(SDL_INIT_SENSOR)) return nullptr;
  int count = 0;
  SDL_SensorID* ids = SDL_GetSensors(&count);
  SDL_Sensor* found = nullptr;
  for (int i = 0; i < count && !found; ++i) {
    if (SDL_GetSensorTypeForID(ids[i]) == SDL_SENSOR_ACCEL) found = SDL_OpenSensor(ids[i]);
  }
  SDL_free(ids);
  if (!found) SDL_QuitSubSystem(SDL_INIT_SENSOR);
  return found;
}

// The window's part no status bar, notch or rounded corner covers, in ImGui's coordinates.
Rect safe_area(SDL_Window* window) {
  SDL_Rect r{};
  if (!SDL_GetWindowSafeArea(window, &r)) SDL_GetWindowSize(window, &r.w, &r.h);
  return {static_cast<float>(r.x), static_cast<float>(r.y), static_cast<float>(r.w), static_cast<float>(r.h)};
}

}  // namespace

DevTools::DevTools(SDL_Window* window, SDL_Renderer* sdl) : window_(window), sdl_(sdl) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;  // never write imgui.ini next to a game
  rescale();
  ImGui_ImplSDL3_InitForSDLRenderer(window, sdl);
  ImGui_ImplSDLRenderer3_Init(sdl);
  accelerometer_ = open_accelerometer();
}

DevTools::~DevTools() {
  if (accelerometer_) {
    SDL_CloseSensor(accelerometer_);
    SDL_QuitSubSystem(SDL_INIT_SENSOR);
  }
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

// Sizes and font for the screen the window is on (devtools/scale); again when it changes.
void DevTools::rescale() {
  const SDL_DisplayID display = SDL_GetDisplayForWindow(window_);
  const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
  const float density = mode ? mode->pixel_density : 1.0f;
  const auto pixels = [density](int points) { return static_cast<int>(static_cast<float>(points) * density); };
  const float scale = tools_scale(mode ? pixels(mode->w) : 0, mode ? pixels(mode->h) : 0, SDL_GetDisplayContentScale(display));
  if (scale == scale_) return;
  scale_ = scale;
  ImGuiStyle style;
  ImGui::StyleColorsDark(&style);
  style.ScaleAllSizes(scale);
  style.FontScaleDpi = scale;  // ImGui 1.92 rasterizes the font at this size: crisp, not stretched
  ImGui::GetStyle() = style;
}

bool DevTools::toggle_requested(const SDL_Event& event) const {
  if (event.type != SDL_EVENT_GAMEPAD_BUTTON_DOWN) return is_toggle(event, false);
  const auto other = event.gbutton.button == SDL_GAMEPAD_BUTTON_BACK ? SDL_GAMEPAD_BUTTON_START
                                                                     : SDL_GAMEPAD_BUTTON_BACK;
  SDL_Gamepad* pad = SDL_GetGamepadFromID(event.gbutton.which);
  return is_toggle(event, pad && SDL_GetGamepadButton(pad, other));
}

// The first accelerometer's samples, while one is open: true when they make a shake.
bool DevTools::shaken(const SDL_Event& event) {
  if (event.type != SDL_EVENT_SENSOR_UPDATE || !accelerometer_) return false;
  if (event.sensor.which != SDL_GetSensorID(accelerometer_)) return false;
  const float* a = event.sensor.data;
  return shake_.sample(a[0], a[1], a[2], SDL_NS_TO_MS(event.sensor.timestamp));
}

bool DevTools::handle(const SDL_Event& event) {
  if (event.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED || event.type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED ||
      event.type == SDL_EVENT_DISPLAY_CURRENT_MODE_CHANGED) {
    rescale();
  }
  if (toggle_requested(event) || shaken(event)) {
    toggle();
    return false;
  }
  if (!open_) return reaches_game(event, {});
  ImGui_ImplSDL3_ProcessEvent(&event);
  const ImGuiIO& io = ImGui::GetIO();
  return reaches_game(event, {io.WantCaptureMouse, io.WantCaptureKeyboard});
}

void DevTools::decorate(render::Frame& frame, const ecs::World* world) const {
  if (!open_ || !panels_.physics_view || !world) return;
  for (core::DrawItem& item : collider_shapes(*world)) frame.shapes.push_back(std::move(item));
}

void DevTools::draw(script::Host& host, FrameStats stats) {
  if (!open_) return;
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  const PanelLayout layout = layout_panels(safe_area(window_), scale_);
  draw_panels(panels_, time_, host, snapshot(host, stats), layout);
  profiler_window(host, stats.drawn, layout.profiler);
  ImGui::Render();
  // ImGui draws in window pixels, not in the game's letterboxed logical size.
  int w = 0;
  int h = 0;
  SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
  SDL_GetRenderLogicalPresentation(sdl_, &w, &h, &mode);
  SDL_SetRenderLogicalPresentation(sdl_, w, h, SDL_LOGICAL_PRESENTATION_DISABLED);
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), sdl_);
  SDL_SetRenderLogicalPresentation(sdl_, w, h, mode);
}

}  // namespace alpha::devtools
