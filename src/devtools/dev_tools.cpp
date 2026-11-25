#include "devtools/dev_tools.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include "devtools/inspector.hpp"
#include "devtools/profiler.hpp"
#include "devtools/routing.hpp"

namespace alpha::devtools {

DevTools::DevTools(SDL_Window* window, SDL_Renderer* sdl) : sdl_(sdl) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().IniFilename = nullptr;  // never write imgui.ini next to a game
  ImGui::StyleColorsDark();
  ImGui_ImplSDL3_InitForSDLRenderer(window, sdl);
  ImGui_ImplSDLRenderer3_Init(sdl);
}

DevTools::~DevTools() {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

bool DevTools::toggle_requested(const SDL_Event& event) const {
  if (event.type != SDL_EVENT_GAMEPAD_BUTTON_DOWN) return is_toggle(event, false);
  const auto other = event.gbutton.button == SDL_GAMEPAD_BUTTON_BACK ? SDL_GAMEPAD_BUTTON_START
                                                                     : SDL_GAMEPAD_BUTTON_BACK;
  SDL_Gamepad* pad = SDL_GetGamepadFromID(event.gbutton.which);
  return is_toggle(event, pad && SDL_GetGamepadButton(pad, other));
}

// Fingers reach the game too: the gesture only needs to see them.
void DevTools::track_fingers(const SDL_Event& event) {
  const SDL_TouchFingerEvent& f = event.tfinger;
  if (event.type == SDL_EVENT_FINGER_DOWN) touch_.down(f.fingerID, f.x, f.y, SDL_NS_TO_MS(f.timestamp));
  else if (event.type == SDL_EVENT_FINGER_MOTION) touch_.move(f.fingerID, f.x, f.y);
  else if (event.type == SDL_EVENT_FINGER_UP || event.type == SDL_EVENT_FINGER_CANCELED) touch_.up(f.fingerID);
}

bool DevTools::handle(const SDL_Event& event) {
  track_fingers(event);
  if (toggle_requested(event)) {
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
  if (touch_.poll(SDL_GetTicks())) toggle();  // a held gesture sends no events: checked every frame
  if (!open_) return;
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
  draw_panels(panels_, time_, host, snapshot(host, stats));
  profiler_window(host, stats.drawn);
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
