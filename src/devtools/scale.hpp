#pragma once

namespace alpha::devtools {

// How big the dev tools draw (ImGui sizes and font) on a screen of `width` x
// `height` pixels with the OS content scale: the larger of that scale and the
// screen's shorter side / 1080, kept within 1..4. Panels then take a similar
// share of any screen: readable on a phone, not huge on a small window.
float tools_scale(int width, int height, float content_scale);

}  // namespace alpha::devtools
