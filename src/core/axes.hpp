#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/input.hpp"

namespace alpha::core {

inline constexpr float kDefaultDeadzone = 0.2f;

// Axial deadzone: |v| <= dz reads 0; beyond it rescales to reach +-1 at +-1.
float axial_deadzone(float v, float dz);
// Radial deadzone for a stick: the vector's length goes through
// axial_deadzone (direction kept), so diagonals are not squared off.
std::array<float, 2> radial_deadzone(float x, float y, float dz);
// Both sticks radial, triggers axial.
AnalogValues with_deadzone(const AnalogValues& raw, float dz);

using AxisId = std::uint16_t;

// Named analog axes, each in [-1, 1]. A source is an analog name
// ("GamepadLeftX", "GamepadLT", ...) or a "Negative/Positive" pair of inputs
// ("A/D", "Left/Right", "GamepadLeft/GamepadRight") that reads -1, 0 or 1.
// The axis reads the source with the largest magnitude (first on ties).
class Axes {
 public:
  // Throws Error unless 0 <= dz < 1.
  void set_deadzone(float dz);
  // Throws Error naming the bad source and the accepted forms.
  AxisId bind(std::string_view axis, std::span<const std::string> sources);
  std::optional<AxisId> find(std::string_view axis) const;
  // Declared names, space separated (for error messages).
  std::string names() const;
  void update(const InputFrame& frame);
  // player: 0..kMaxPlayers-1, or kAnyPlayer (the player with the largest magnitude).
  float value(AxisId id, std::size_t player = kAnyPlayer) const;
  // Overrides one player's value until the next update (headless tests); clamped to [-1, 1].
  void set(AxisId id, std::size_t player, float v);

 private:
  struct Source {
    std::optional<Analog> analog;
    InputCode neg = 0;
    InputCode pos = 0;
  };
  struct Axis {
    std::string name;
    std::vector<Source> sources;
    std::array<float, kMaxPlayers> values{};
  };
  static float resolve(const Axis& axis, const InputSet& held, const AnalogValues& analog);

  std::vector<Axis> axes_;
  float deadzone_ = kDefaultDeadzone;
};

}  // namespace alpha::core
