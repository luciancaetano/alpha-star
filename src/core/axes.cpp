#include "core/axes.hpp"

#include <algorithm>
#include <cmath>

#include "core/error.hpp"
#include "core/index_of.hpp"
#include "core/touch_pad.hpp"

namespace alpha::core {

namespace {

std::string forms() {
  std::string msg = "A source is an analog axis (";
  for (std::string_view n : analog_names()) msg += std::string(n) + (n == analog_names().back() ? "" : " ");
  return msg + ") or a \"Negative/Positive\" pair of inputs such as \"A/D\" or \"Left/Right\".";
}

[[noreturn]] void bad_source(std::string_view axis, std::string_view source, std::string_view why,
                             const TouchPad* touch) {
  const std::string sticks = touch ? touch->axis_names() : "";
  throw Error("axis \"" + std::string(axis) + "\": " + std::string(why) + " \"" + std::string(source) + "\"" +
              (sticks.empty() ? "" : "; touch stick sources: " + sticks) + ". " + forms());
}

InputCode pair_input(std::string_view axis, std::string_view name, const TouchPad* touch) {
  const auto code = touch ? touch->input(name) : parse_input(name);
  if (!code) bad_source(axis, name, "unknown input", touch);
  return *code;
}

// The value with the larger magnitude; `best` on ties.
float stronger(float best, float v) { return std::fabs(v) > std::fabs(best) ? v : best; }

float digital(const InputSet& held, InputCode neg, InputCode pos) {
  return (held.test(pos) ? 1.0f : 0.0f) - (held.test(neg) ? 1.0f : 0.0f);
}

}  // namespace

float axial_deadzone(float v, float dz) {
  const float m = std::fabs(v);
  if (m <= dz) return 0.0f;
  const float scaled = std::min(1.0f, (m - dz) / (1.0f - dz));
  return v < 0.0f ? -scaled : scaled;
}

std::array<float, 2> radial_deadzone(float x, float y, float dz) {
  const float len = std::hypot(x, y);
  if (len <= dz) return {0.0f, 0.0f};
  const float k = axial_deadzone(len, dz) / len;
  return {x * k, y * k};
}

AnalogValues with_deadzone(const AnalogValues& raw, float dz) {
  const auto left = radial_deadzone(raw[0], raw[1], dz);
  const auto right = radial_deadzone(raw[2], raw[3], dz);
  return {left[0], left[1], right[0], right[1], axial_deadzone(raw[4], dz), axial_deadzone(raw[5], dz)};
}

void Axes::set_deadzone(float dz) {
  if (!(dz >= 0.0f && dz < 1.0f)) {
    throw Error("config.deadzone must be a number >= 0 and < 1 (default 0.2), got " + std::to_string(dz));
  }
  deadzone_ = dz;
}

Axes::Source Axes::parse_source(std::string_view axis, std::string_view src, const TouchPad* touch) {
  if (const auto analog = parse_analog(src)) return {analog, std::nullopt, 0, 0};
  if (const auto stick = touch ? touch->axis(src) : std::nullopt) return {std::nullopt, stick, 0, 0};
  const auto slash = src.find('/');
  if (slash == std::string_view::npos || src.find('/', slash + 1) != std::string_view::npos) {
    bad_source(axis, src, "unknown source", touch);
  }
  return {std::nullopt, std::nullopt, pair_input(axis, src.substr(0, slash), touch),
          pair_input(axis, src.substr(slash + 1), touch)};
}

AxisId Axes::bind(std::string_view axis, std::span<const std::string> sources, const TouchPad* touch) {
  Axis a{std::string(axis), {}, {}};
  for (const std::string& src : sources) a.sources.push_back(parse_source(axis, src, touch));
  axes_.push_back(std::move(a));
  return static_cast<AxisId>(axes_.size() - 1);
}

std::optional<AxisId> Axes::find(std::string_view axis) const { return index_by_name<AxisId>(axes_, axis); }

std::string Axes::names() const { return names_of(axes_); }

float Axes::resolve(const Axis& axis, const InputSet& held, const AnalogValues& analog, const TouchAnalog& touch) {
  float best = 0.0f;
  for (const Source& s : axis.sources) {
    const float v = s.analog  ? analog[static_cast<std::size_t>(*s.analog)]
                    : s.touch ? touch[*s.touch]
                              : digital(held, s.neg, s.pos);
    best = stronger(best, v);
  }
  return std::clamp(best, -1.0f, 1.0f);
}

void Axes::update(const InputFrame& frame) {
  for (std::size_t p = 0; p < kMaxPlayers; ++p) {
    const PlayerInput& in = frame.players[p];
    const InputSet held = in.held | analog_buttons(in.analog);
    const AnalogValues analog = with_deadzone(in.analog, deadzone_);
    for (Axis& a : axes_) a.values[p] = a.held[p].value_or(resolve(a, held, analog, in.touch));
  }
}

float Axes::value(AxisId id, std::size_t player) const {
  const auto& values = axes_[id].values;
  if (player != kAnyPlayer) return values[player];
  float best = 0.0f;
  for (float v : values) best = stronger(best, v);
  return best;
}

void Axes::hold(AxisId id, std::size_t player, std::optional<float> v) {
  auto& held = axes_[id].held[player];
  held = v ? std::optional<float>(std::clamp(*v, -1.0f, 1.0f)) : std::nullopt;
}

void Axes::set(AxisId id, std::size_t player, float v) { axes_[id].values[player] = std::clamp(v, -1.0f, 1.0f); }

}  // namespace alpha::core
