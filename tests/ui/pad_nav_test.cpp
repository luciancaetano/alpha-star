#include <doctest/doctest.h>

#include "ui/pad_nav.hpp"

using alpha::ui::Nav;
using alpha::ui::PadNav;

TEST_CASE("pad nav: D-pad moves once on press, repeats after the delay while held") {
  PadNav pad;
  CHECK(pad.button(Nav::Next, true, 1.0) == Nav::Next);
  CHECK(pad.tick(1.0) == Nav::None);
  CHECK(pad.tick(1.0 + PadNav::kDelay - 0.01) == Nav::None);
  CHECK(pad.tick(1.0 + PadNav::kDelay) == Nav::Next);
  CHECK(pad.tick(1.0 + PadNav::kDelay + 0.05) == Nav::None);
  CHECK(pad.tick(1.0 + PadNav::kDelay + PadNav::kRepeat) == Nav::Next);
  CHECK(pad.button(Nav::Next, false, 2.0) == Nav::None);
  CHECK(pad.tick(5.0) == Nav::None);
}

TEST_CASE("pad nav: a long frame never bursts several moves") {
  PadNav pad;
  pad.button(Nav::Prev, true, 0.0);
  CHECK(pad.tick(10.0) == Nav::Prev);
  CHECK(pad.tick(10.0) == Nav::None);
}

TEST_CASE("pad nav: left stick counts past the deadzone on its dominant axis") {
  PadNav pad;
  CHECK(pad.stick(0.0f, 0.4f, 0.0) == Nav::None);   // inside the deadzone
  CHECK(pad.stick(0.2f, 0.8f, 0.0) == Nav::Next);   // down
  CHECK(pad.stick(0.2f, 0.9f, 0.1) == Nav::None);   // still held: no new move
  CHECK(pad.tick(PadNav::kDelay) == Nav::Next);     // repeat while held
  CHECK(pad.stick(0.0f, 0.1f, 0.5) == Nav::None);   // released
  CHECK(pad.tick(2.0) == Nav::None);
  CHECK(pad.stick(-0.9f, 0.3f, 3.0) == Nav::Prev);  // left
  CHECK(pad.stick(0.0f, -0.7f, 3.1) == Nav::None);  // up: same direction, no new move
}
