#include "ui/pad_nav.hpp"

#include <cmath>

namespace alpha::ui {

namespace {

Nav stick_dir(float x, float y) {
  const float main = std::fabs(y) >= std::fabs(x) ? y : x;
  if (std::fabs(main) < PadNav::kDeadzone) return Nav::None;
  return main > 0 ? Nav::Next : Nav::Prev;
}

}  // namespace

Nav PadNav::hold(Nav dir, double now) {
  if (dir == held_) return Nav::None;
  held_ = dir;
  next_ = now + kDelay;
  return dir;
}

Nav PadNav::button(Nav dir, bool down, double now) {
  if (down) return hold(dir, now);
  if (dir == held_) held_ = Nav::None;
  return Nav::None;
}

Nav PadNav::stick(float x, float y, double now) {
  const Nav dir = stick_dir(x, y);
  if (dir == stick_) return Nav::None;
  const Nav was = stick_;
  stick_ = dir;
  if (dir == Nav::None) return button(was, false, now);
  return hold(dir, now);
}

Nav PadNav::tick(double now) {
  if (held_ == Nav::None || now < next_) return Nav::None;
  next_ += kRepeat;
  if (next_ <= now) next_ = now + kRepeat;  // a long frame never bursts moves
  return held_;
}

}  // namespace alpha::ui
