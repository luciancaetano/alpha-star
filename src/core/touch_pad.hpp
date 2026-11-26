#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/input.hpp"
#include "core/touch_button.hpp"
#include "core/touch_stick.hpp"

namespace alpha::core {

// What one fixed step of fingers gave the game: touch control inputs and
// stick axes per player, plus the fingers no control captured (`free`: they
// hold player 1's "Touch", "TouchLeft", "TouchRight" and "MouseLeft").
struct TouchFrame {
  std::array<InputSet, kMaxPlayers> held;
  std::array<TouchAnalog, kMaxPlayers> analog{};
  std::vector<Finger> free;
};

// A stick or button as it is now: the finger holding it (if any), and for a
// stick where its base and knob are and its value (-1..1 per axis).
struct StickState {
  std::optional<std::uint64_t> finger;
  Vec2 center;
  Vec2 knob;
  Vec2 value;
};
struct ButtonState {
  std::optional<std::uint64_t> finger;
};

// The game's touch controls (config.touchSticks / config.touchButtons): each
// new finger is captured by the first control under it (buttons, then sticks
// in declaration order) and drives only that control until it lifts. Inert
// (nothing captured) unless `active` (mobile). Names: a stick "move" makes
// TouchMoveX / TouchMoveY (axes), TouchMoveX- / X+ / Y- / Y+ and
// TouchMoveActive (inputs); a button "jump" makes the input TouchJump.
class TouchPad {
 public:
  // Throws Error on too many controls, a bad name or one used twice.
  void configure(std::vector<StickConfig> sticks, std::vector<ButtonConfig> buttons, bool active);
  void set_screen(const TouchScreen& screen) { screen_ = screen; }
  const TouchScreen& screen() const { return screen_; }
  bool active() const { return active_; }
  TouchFrame update(const std::vector<Finger>& fingers);
  // Pause / focus loss: every control lets go and reads 0; the fingers that
  // held them are ignored until they lift.
  void release_all();
  // A scene started: every control lets go (as release_all); only controls
  // whose `scenes` list it (or is empty) capture and draw from now on.
  void enter_scene(std::string scene);
  // The control exists in the running scene.
  bool here(const std::vector<std::string>& scenes) const;

  // Any input name: the fixed ones (parse_input) and this game's touch ones.
  std::optional<InputCode> input(std::string_view name) const;
  // A stick axis source ("TouchMoveX"): index into PlayerInput::touch.
  std::optional<std::size_t> axis(std::string_view name) const;
  // This game's touch names, space separated (for error messages).
  std::string input_names() const;
  std::string axis_names() const;

  const std::vector<StickConfig>& sticks() const { return sticks_; }
  const std::vector<StickState>& stick_states() const { return stick_states_; }
  const std::vector<ButtonConfig>& buttons() const { return buttons_; }
  const std::vector<ButtonState>& button_states() const { return button_states_; }

 private:
  void capture(const Finger& f);
  bool owned(const Finger& f, TouchFrame& out);
  void drive_stick(std::size_t i, const Finger& f, TouchFrame& out);
  void let_go(std::uint64_t id);
  void drop_missing(const std::vector<Finger>& fingers);
  bool ignored(const Finger& f);

  std::vector<StickConfig> sticks_;
  std::vector<StickState> stick_states_;
  std::vector<ButtonConfig> buttons_;
  std::vector<ButtonState> button_states_;
  std::vector<std::uint64_t> ignored_;  // held through release_all
  TouchScreen screen_{{{0.0f, 0.0f}, {640.0f, 360.0f}}, {640.0f, 360.0f}};
  bool active_ = false;
  std::string scene_;
};

// "move" -> "Move": a control's name as it appears in its sources.
std::string pascal(std::string_view name);

}  // namespace alpha::core
