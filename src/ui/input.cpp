#include "ui/input.hpp"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi_Platform_SDL.h>

namespace alpha::ui {

namespace {

double seconds(Uint64 ns) { return static_cast<double>(ns) / 1e9; }

bool text_focused(Rml::Context& context) {
  const Rml::Element* focus = context.GetFocusElement();
  if (!focus) return false;
  const std::string& tag = focus->GetTagName();
  return tag == "textarea" || (tag == "input" && focus->GetAttribute<Rml::String>("type", "text") == "text");
}

bool mouse_button(Ui& ui, const SDL_MouseButtonEvent& b) {
  Rml::Context& context = ui.context();
  ui.used_gamepad(false);
  context.Update();  // layout current before hit testing
  context.ProcessMouseMove(static_cast<int>(b.x), static_cast<int>(b.y), RmlSDL::GetKeyModifierState());
  const int button = RmlSDL::ConvertMouseButton(b.button);
  if (b.down) context.ProcessMouseButtonDown(button, RmlSDL::GetKeyModifierState());
  else context.ProcessMouseButtonUp(button, RmlSDL::GetKeyModifierState());
  return ui.interactive(context.GetHoverElement());
}

// Down: consumed if the UI acted (focus moved, focused element clicked, back
// handled) or a text field has focus. Escape is "back".
bool key_down(Ui& ui, Rml::Input::KeyIdentifier key, int modifiers) {
  Rml::Context& context = ui.context();
  context.Update();
  const bool acted = !context.ProcessKeyDown(key, modifiers);
  const bool backed = key == Rml::Input::KI_ESCAPE && ui.back();
  return acted || backed || text_focused(context);
}

bool keyboard(Ui& ui, const SDL_KeyboardEvent& k) {
  ui.used_gamepad(false);
  const Rml::Input::KeyIdentifier key = RmlSDL::ConvertKey(static_cast<int>(k.key));
  if (k.down) return key_down(ui, key, RmlSDL::GetKeyModifierState());
  ui.context().ProcessKeyUp(key, RmlSDL::GetKeyModifierState());
  return text_focused(ui.context());
}

Nav dpad(Uint8 button) {
  if (button == SDL_GAMEPAD_BUTTON_DPAD_DOWN || button == SDL_GAMEPAD_BUTTON_DPAD_RIGHT) return Nav::Next;
  if (button == SDL_GAMEPAD_BUTTON_DPAD_UP || button == SDL_GAMEPAD_BUTTON_DPAD_LEFT) return Nav::Prev;
  return Nav::None;
}

// A (South) = Enter on the focused element, B (East) = Escape (back).
Rml::Input::KeyIdentifier pad_key(Uint8 button) {
  if (button == SDL_GAMEPAD_BUTTON_SOUTH) return Rml::Input::KI_RETURN;
  if (button == SDL_GAMEPAD_BUTTON_EAST) return Rml::Input::KI_ESCAPE;
  return Rml::Input::KI_UNKNOWN;
}

bool pad_button(Ui& ui, const SDL_GamepadButtonEvent& b) {
  ui.used_gamepad(true);
  if (const Nav nav = dpad(b.button); nav != Nav::None) {
    return ui.navigate(ui.pad().button(nav, b.down, seconds(b.timestamp)));
  }
  const Rml::Input::KeyIdentifier key = pad_key(b.button);
  if (key == Rml::Input::KI_UNKNOWN) return false;
  if (b.down) return key_down(ui, key, 0);
  ui.context().ProcessKeyUp(key, 0);
  return false;
}

bool pad_axis(Ui& ui, const SDL_GamepadAxisEvent& a) {
  if (a.axis != SDL_GAMEPAD_AXIS_LEFTX && a.axis != SDL_GAMEPAD_AXIS_LEFTY) return false;
  ui.used_gamepad(true);
  ui.stick[a.axis == SDL_GAMEPAD_AXIS_LEFTX ? 0 : 1] = static_cast<float>(a.value) / 32767.0f;
  return ui.navigate(ui.pad().stick(ui.stick[0], ui.stick[1], seconds(a.timestamp)));
}

bool pointer(Ui& ui, const SDL_Event& event) {
  Rml::Context& context = ui.context();
  if (event.type == SDL_EVENT_MOUSE_WHEEL) {
    return !context.ProcessMouseWheel(-event.wheel.y, RmlSDL::GetKeyModifierState());
  }
  if (event.type != SDL_EVENT_MOUSE_MOTION) return mouse_button(ui, event.button);
  context.ProcessMouseMove(static_cast<int>(event.motion.x), static_cast<int>(event.motion.y),
                           RmlSDL::GetKeyModifierState());
  return false;
}

}  // namespace

bool consumes(Ui& ui, const SDL_Event& event) {
  // SDL3 numbers motion, button down/up and wheel consecutively.
  if (event.type >= SDL_EVENT_MOUSE_MOTION && event.type <= SDL_EVENT_MOUSE_WHEEL) return pointer(ui, event);
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: return keyboard(ui, event.key);
    case SDL_EVENT_TEXT_INPUT:
      ui.context().ProcessTextInput(Rml::String(event.text.text));
      return text_focused(ui.context());
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP: return pad_button(ui, event.gbutton);
    case SDL_EVENT_GAMEPAD_AXIS_MOTION: return pad_axis(ui, event.gaxis);
    default: return false;
  }
}

}  // namespace alpha::ui
