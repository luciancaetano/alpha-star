#include "core/input.hpp"

#include <array>

#include "core/error.hpp"
#include "core/index_of.hpp"

namespace alpha::core {

namespace {

constexpr std::array<std::string_view, 64> kNames{
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R",
    "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "Space", "Enter", "Escape", "Tab", "Backspace", "Left", "Right", "Up", "Down",
    "LeftShift", "RightShift", "LeftCtrl", "RightCtrl",
    "MouseLeft", "MouseRight", "MouseMiddle",
    "GamepadA", "GamepadB", "GamepadX", "GamepadY", "GamepadStart", "GamepadBack",
    "GamepadUp", "GamepadDown", "GamepadLeft", "GamepadRight", "GamepadLB", "GamepadRB"};
static_assert(kNames.size() <= kMaxInputs);

std::string unknown_input(std::string_view action, std::string_view input) {
  std::string msg = "action \"" + std::string(action) + "\": unknown input \"" +
                    std::string(input) + "\". Valid inputs:";
  for (std::string_view n : kNames) msg += " " + std::string(n);
  return msg;
}

}  // namespace

std::span<const std::string_view> input_names() { return kNames; }

std::optional<InputCode> parse_input(std::string_view name) {
  return index_of<InputCode>(kNames, name);
}

ActionId Actions::bind(std::string_view action, std::span<const std::string> inputs) {
  Action a{std::string(action), {}, false, false};
  for (const std::string& input : inputs) {
    const auto code = parse_input(input);
    if (!code) throw Error(unknown_input(action, input));
    a.inputs.set(*code);
  }
  actions_.push_back(std::move(a));
  return static_cast<ActionId>(actions_.size() - 1);
}

std::optional<ActionId> Actions::find(std::string_view action) const {
  for (std::size_t i = 0; i < actions_.size(); ++i) {
    if (actions_[i].name == action) return static_cast<ActionId>(i);
  }
  return std::nullopt;
}

void Actions::update(const InputSet& held) {
  for (Action& a : actions_) {
    a.before = a.now;
    a.now = (a.inputs & held).any();
  }
}

}  // namespace alpha::core
