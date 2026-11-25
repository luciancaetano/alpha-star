#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace alpha::core {

// Device-independent input code: index into input_names().
using InputCode = std::uint16_t;
inline constexpr std::size_t kMaxInputs = 128;
using InputSet = std::bitset<kMaxInputs>;

// Every input name scripts may bind: "A".."Z", "0".."9", "Space", "Enter",
// arrows, "MouseLeft", "GamepadA", ..., and the digital view of the analog
// axes: "GamepadLeftX-" / "GamepadLeftX+" (stick past kPressThreshold),
// "GamepadLT" / "GamepadRT" (trigger past kPressThreshold).
std::span<const std::string_view> input_names();
std::optional<InputCode> parse_input(std::string_view name);

// Players: index 0 is player 1 (keyboard, mouse and the first gamepad).
inline constexpr std::size_t kMaxPlayers = 4;
inline constexpr std::size_t kAnyPlayer = kMaxPlayers;

// Analog gamepad axes, raw: sticks in [-1, 1] (+x right, +y down), triggers in [0, 1].
enum class Analog : std::uint8_t { LeftX, LeftY, RightX, RightY, LT, RT };
inline constexpr std::size_t kAnalogCount = 6;
using AnalogValues = std::array<float, kAnalogCount>;
inline constexpr float kPressThreshold = 0.5f;

// "GamepadLeftX", "GamepadLeftY", "GamepadRightX", "GamepadRightY", "GamepadLT", "GamepadRT".
std::span<const std::string_view> analog_names();
std::optional<Analog> parse_analog(std::string_view name);
// The digital inputs the analog axes hold ("GamepadLeftY-" = left stick up).
InputSet analog_buttons(const AnalogValues& raw);

struct PlayerInput {
  InputSet held;
  AnalogValues analog{};
};

// What the devices did during one fixed step. `connected` = players with a
// device: player 1 always (keyboard), plus the highest occupied gamepad slot.
struct InputFrame {
  std::array<PlayerInput, kMaxPlayers> players{};
  std::size_t connected = 1;
};

using ActionId = std::uint16_t;

// Named actions mapped to device inputs. Game logic only sees actions.
class Actions {
 public:
  // Throws Error naming the unknown input and listing the valid ones.
  ActionId bind(std::string_view action, std::span<const std::string> inputs);
  std::optional<ActionId> find(std::string_view action) const;
  // Call once per fixed step with what every player held (or pressed) during it.
  void update(const InputFrame& frame);

  // player: 0..kMaxPlayers-1, or kAnyPlayer (true if it holds for any player).
  bool down(ActionId id, std::size_t player = kAnyPlayer) const;
  bool pressed(ActionId id, std::size_t player = kAnyPlayer) const;
  bool released(ActionId id, std::size_t player = kAnyPlayer) const;
  // The device inputs bound to an action (to press it without a device).
  const InputSet& inputs(ActionId id) const { return actions_[id].inputs; }

 private:
  using Players = std::bitset<kMaxPlayers>;
  struct Action {
    std::string name;
    InputSet inputs;
    Players now;
    Players before;
  };
  std::vector<Action> actions_;
};

}  // namespace alpha::core
