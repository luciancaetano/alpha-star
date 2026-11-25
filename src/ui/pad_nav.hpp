#pragma once

namespace alpha::ui {

// A focus move in the game UI: next / previous focusable element.
enum class Nav { None, Next, Prev };

// Gamepad focus navigation (pure): the D-pad and the left stick hold a
// direction; holding moves once at once, again after kDelay, then every
// kRepeat. Down/right = Next, up/left = Prev. The stick counts past kDeadzone
// on its dominant axis. Times in seconds.
class PadNav {
 public:
  static constexpr float kDeadzone = 0.5f;
  static constexpr double kDelay = 0.4;
  static constexpr double kRepeat = 0.12;

  // D-pad: `dir` went down (or up). Returns the move to make now.
  Nav button(Nav dir, bool down, double now);
  // Left stick moved to (x, y), each in [-1, 1]. Returns the move to make now.
  Nav stick(float x, float y, double now);
  // Called every frame: the repeat move due at `now`, if any.
  Nav tick(double now);

 private:
  Nav hold(Nav dir, double now);

  Nav held_ = Nav::None;
  Nav stick_ = Nav::None;
  double next_ = 0.0;
};

}  // namespace alpha::ui
