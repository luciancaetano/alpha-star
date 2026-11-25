#pragma once

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
// arrows, "MouseLeft", "GamepadA", ...
std::span<const std::string_view> input_names();
std::optional<InputCode> parse_input(std::string_view name);

using ActionId = std::uint16_t;

// Named actions mapped to device inputs. Game logic only sees actions.
class Actions {
 public:
  // Throws Error naming the unknown input and listing the valid ones.
  ActionId bind(std::string_view action, std::span<const std::string> inputs);
  std::optional<ActionId> find(std::string_view action) const;
  // Call once per fixed step with the inputs held (or pressed) during it.
  void update(const InputSet& held);

  bool down(ActionId id) const { return actions_[id].now; }
  bool pressed(ActionId id) const { return actions_[id].now && !actions_[id].before; }
  bool released(ActionId id) const { return !actions_[id].now && actions_[id].before; }

 private:
  struct Action {
    std::string name;
    InputSet inputs;
    bool now = false;
    bool before = false;
  };
  std::vector<Action> actions_;
};

}  // namespace alpha::core
