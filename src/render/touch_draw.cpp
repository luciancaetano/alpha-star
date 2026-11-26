#include "render/touch_draw.hpp"

#include <algorithm>

namespace alpha::render {

namespace {

constexpr core::Color kBaseFill{40, 40, 40, 45};
constexpr core::Color kRing{230, 230, 230, 90};
constexpr core::Color kKnob{200, 200, 200, 100};
constexpr core::Color kButtonUp{60, 60, 60, 70};
constexpr core::Color kButtonDown{150, 150, 150, 110};

core::DrawItem circle(core::Vec2 at, float radius, core::Color fill, std::optional<core::Color> ring, float alpha) {
  core::ShapeSpec shape(core::Circle{radius}, fill, ring);
  shape.alpha = alpha;
  shape.smooth = true;
  return {shape, at};
}

core::DrawItem picture(const core::Picture& pic, core::Vec2 at, float size, core::Color tint, float alpha) {
  core::ShapeSpec shape(core::Rect{{size, size}}, {}, std::nullopt, pic.image);
  shape.src = pic.region;
  shape.tint = tint;
  shape.alpha = alpha;
  return {shape, at};
}

float or_default(float size, float fallback) { return size > 0.0f ? size : fallback; }

const core::Picture& shown(const core::Picture& rest, const std::optional<core::Picture>& pressed, bool on) {
  return on && pressed ? *pressed : rest;
}

// Where a stick is drawn this frame and how opaque.
struct StickPose {
  core::Vec2 center;
  core::Vec2 knob;
  bool touching = false;
  float alpha = 1.0f;
};

void skinned_stick(const core::StickConfig& cfg, const core::StickSkin& skin, const StickPose& p, TouchOverlay& out) {
  const float base = or_default(skin.base_size, cfg.radius * 2.0f);
  const float knob = or_default(skin.knob_size, cfg.radius);
  out.shapes.push_back(picture(shown(skin.base, skin.base_pressed, p.touching), p.center, base, skin.tint, p.alpha));
  out.shapes.push_back(picture(shown(skin.knob, skin.knob_pressed, p.touching), p.knob, knob, skin.tint, p.alpha));
}

void draw_stick(const core::StickConfig& cfg, const core::StickState& st, const core::TouchScreen& screen,
                TouchOverlay& out) {
  const bool touching = st.finger.has_value();
  if (!touching && !cfg.always_visible) return;
  const core::Vec2 center = touching ? st.center : core::stick_anchor(cfg, screen);
  const StickPose pose{center, touching ? st.knob : center, touching, touching ? 1.0f : cfg.opacity};
  if (cfg.skin) return skinned_stick(cfg, *cfg.skin, pose, out);
  out.shapes.push_back(circle(pose.center, cfg.radius, kBaseFill, kRing, pose.alpha));
  out.shapes.push_back(circle(pose.knob, cfg.radius * 0.5f, kKnob, std::nullopt, pose.alpha));
}

// Without its own `down` image a pressed skin shows `up` darkened.
void skinned_button(const core::ButtonConfig& cfg, const core::ButtonSkin& skin, core::Vec2 at, bool down, float alpha,
                    TouchOverlay& out) {
  const core::Color tint = down && !skin.down ? core::scaled(skin.tint, 0.6f) : skin.tint;
  out.shapes.push_back(picture(shown(skin.up, skin.down, down), at, or_default(skin.size, cfg.radius * 2.0f), tint, alpha));
}

void draw_button(const core::ButtonConfig& cfg, const core::ButtonState& st, const core::TouchScreen& screen,
                 TouchOverlay& out) {
  if (!cfg.visible) return;
  const bool down = st.finger.has_value();
  const core::Vec2 at = core::in_safe(screen, cfg.pos);
  const float alpha = down ? 1.0f : cfg.opacity;
  if (cfg.skin) return skinned_button(cfg, *cfg.skin, at, down, alpha, out);
  out.shapes.push_back(circle(at, cfg.radius, down ? kButtonDown : kButtonUp, kRing, alpha));
  if (cfg.label.empty()) return;
  const auto a = static_cast<std::uint8_t>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
  out.labels.push_back({cfg.label, at, cfg.radius * 0.8f, {255, 255, 255, a}, core::Align::Center});
}

}  // namespace

TouchOverlay touch_overlay(const core::TouchPad& pad) {
  TouchOverlay out;
  if (!pad.active()) return out;
  for (std::size_t i = 0; i < pad.sticks().size(); ++i) {
    if (pad.here(pad.sticks()[i].scenes)) draw_stick(pad.sticks()[i], pad.stick_states()[i], pad.screen(), out);
  }
  for (std::size_t j = 0; j < pad.buttons().size(); ++j) {
    if (pad.here(pad.buttons()[j].scenes)) draw_button(pad.buttons()[j], pad.button_states()[j], pad.screen(), out);
  }
  return out;
}

}  // namespace alpha::render
