#include "core/input.hpp"

#include <array>

#include "core/error.hpp"
#include "core/index_of.hpp"

namespace alpha::core {

namespace {

constexpr std::array<std::string_view, 76> kNames{
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R",
    "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "Space", "Enter", "Escape", "Tab", "Backspace", "Left", "Right", "Up", "Down",
    "LeftShift", "RightShift", "LeftCtrl", "RightCtrl",
    "MouseLeft", "MouseRight", "MouseMiddle",
    "GamepadA", "GamepadB", "GamepadX", "GamepadY", "GamepadStart", "GamepadBack",
    "GamepadUp", "GamepadDown", "GamepadLeft", "GamepadRight", "GamepadLB", "GamepadRB",
    "GamepadLS", "GamepadRS", "GamepadLT", "GamepadRT",
    "GamepadLeftX-", "GamepadLeftX+", "GamepadLeftY-", "GamepadLeftY+",
    "GamepadRightX-", "GamepadRightX+", "GamepadRightY-", "GamepadRightY+"};
static_assert(kNames.size() <= kMaxInputs);

std::string unknown_input(std::string_view action, std::string_view input) {
  std::string msg = "action \"" + std::string(action) + "\": unknown input \"" +
                    std::string(input) + "\". Valid inputs:";
  for (std::string_view n : kNames) msg += " " + std::string(n);
  return msg;
}

constexpr std::array<std::string_view, kAnalogCount> kAnalog{
    "GamepadLeftX", "GamepadLeftY", "GamepadRightX", "GamepadRightY", "GamepadLT", "GamepadRT"};

}  // namespace

std::span<const std::string_view> input_names() { return kNames; }

std::optional<InputCode> parse_input(std::string_view name) {
  return index_of<InputCode>(kNames, name);
}

std::span<const std::string_view> analog_names() { return kAnalog; }

std::optional<Analog> parse_analog(std::string_view name) { return index_of<Analog>(kAnalog, name); }

InputSet analog_buttons(const AnalogValues& raw) {
  InputSet held;
  // Sticks: kNames lists "<axis>-", "<axis>+" in analog order.
  const InputCode first = *parse_input("GamepadLeftX-");
  for (std::size_t i = 0; i < 4; ++i) {
    held.set(first + 2 * i, raw[i] < -kPressThreshold);
    held.set(first + 2 * i + 1, raw[i] > kPressThreshold);
  }
  held.set(*parse_input("GamepadLT"), raw[4] > kPressThreshold);
  held.set(*parse_input("GamepadRT"), raw[5] > kPressThreshold);
  return held;
}

ActionId Actions::bind(std::string_view action, std::span<const std::string> inputs) {
  Action a{std::string(action), {}, {}, {}};
  for (const std::string& input : inputs) {
    const auto code = parse_input(input);
    if (!code) throw Error(unknown_input(action, input));
    a.inputs.set(*code);
  }
  actions_.push_back(std::move(a));
  return static_cast<ActionId>(actions_.size() - 1);
}

std::optional<ActionId> Actions::find(std::string_view action) const {
  return index_by_name<ActionId>(actions_, action);
}

void Actions::update(const InputFrame& frame) {
  std::array<InputSet, kMaxPlayers> held;
  for (std::size_t p = 0; p < kMaxPlayers; ++p) {
    held[p] = frame.players[p].held | analog_buttons(frame.players[p].analog);
  }
  for (Action& a : actions_) {
    a.before = a.now;
    for (std::size_t p = 0; p < kMaxPlayers; ++p) a.now.set(p, (a.inputs & held[p]).any());
  }
}

namespace {

bool holds(std::bitset<kMaxPlayers> players, std::size_t player) {
  return player == kAnyPlayer ? players.any() : players.test(player);
}

}  // namespace

bool Actions::down(ActionId id, std::size_t player) const { return holds(actions_[id].now, player); }

bool Actions::pressed(ActionId id, std::size_t player) const {
  return holds(actions_[id].now & ~actions_[id].before, player);
}

bool Actions::released(ActionId id, std::size_t player) const {
  return holds(~actions_[id].now & actions_[id].before, player);
}

}  // namespace alpha::core
