#pragma once

#include <cstdint>
#include <optional>

#include "core/vec2.hpp"

// 2D camera: which part of the world the screen shows. Pure; the World owns
// one per scene and steps it on the fixed step (deterministic shake).
namespace alpha::core {

// Axis-aligned box in world pixels.
struct Box {
  Vec2 min;
  Vec2 max;
};

// What the renderer needs: world point `center` lands at the middle of a
// `size` screen, scaled by `zoom`, the world turned by -`rotation` degrees.
struct View {
  Vec2 center;
  float zoom = 1.0f;
  float rotation = 0.0f;  // degrees
  Vec2 size;
};

// v turned by `degrees` (+y down, so positive turns clockwise on screen).
Vec2 rotated(Vec2 v, float degrees);
Vec2 to_screen(const View& view, Vec2 world);
Vec2 to_world(const View& view, Vec2 screen);
// The world box that contains everything on screen (rotation included).
Box visible_area(const View& view);
// a -> b by t in [0, 1] (render interpolation).
View blend(const View& a, const View& b, float t);
bool overlaps(const Box& a, const Box& b);
// How a layer with `parallax` sees the world: the camera's offset from the
// identity camera (and its zoom and rotation) scaled by the factor, so 0 is
// fixed to the screen and 1 is the camera itself.
View parallax_view(const View& view, float parallax);

struct Follow {
  float lerp = 1.0f;  // fraction of the remaining distance covered per fixed step, (0, 1]
  Vec2 deadzone;      // the target moves freely inside this box around the center
};

struct Camera {
  Vec2 size;          // screen size in game pixels
  Vec2 pos;           // world point at the screen center (without shake)
  float zoom = 1.0f;  // > 0
  float rotation = 0.0f;
  std::optional<Follow> follow;
  std::optional<Box> bounds;  // the visible area stays inside (centered when larger)
  float shake_intensity = 0.0f;  // pixels at the start of the shake
  float shake_total = 0.0f;
  float shake_left = 0.0f;
  std::uint32_t noise = 0x9E3779B9u;  // shake noise state: same steps, same shake
  Vec2 shake_offset;
};

// Identity camera: pos = size / 2, so world pixels are screen pixels.
Camera make_camera(Vec2 size);
void start_shake(Camera& camera, float intensity, float seconds);
// One fixed step: move toward `target` (when following one), clamp to
// bounds, advance the shake.
void step_camera(Camera& camera, std::optional<Vec2> target, float dt);
View view_of(const Camera& camera);

}  // namespace alpha::core
