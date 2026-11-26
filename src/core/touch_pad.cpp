#include "core/touch_pad.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "core/error.hpp"
#include "core/limits.hpp"

namespace alpha::core {

namespace {

// Per stick, kStickInputs codes in this order; buttons follow all sticks.
constexpr std::array<std::string_view, 5> kStickSuffix{"X-", "X+", "Y-", "Y+", "Active"};
constexpr std::size_t kStickInputs = kStickSuffix.size();
constexpr std::size_t kButtonBase = kTouchInputBase + kMaxTouchSticks * kStickInputs;
static_assert(kButtonBase + kMaxTouchButtons <= kMaxInputs);
static_assert(2 * kMaxTouchSticks == kTouchAnalogCount);

InputCode stick_code(std::size_t stick, std::size_t k) {
  return static_cast<InputCode>(kTouchInputBase + stick * kStickInputs + k);
}

InputCode button_code(std::size_t button) { return static_cast<InputCode>(kButtonBase + button); }

std::string source_of(std::string_view name) { return "Touch" + pascal(name); }

bool valid_name(std::string_view name) {
  return !name.empty() && std::isalpha(static_cast<unsigned char>(name[0])) &&
         std::all_of(name.begin(), name.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)); });
}

// Throws on a bad name, or one whose Touch<Name> is a fixed input or already taken.
void check_name(const std::string& name, const char* table, std::vector<std::string>& seen) {
  const std::string source = source_of(name);
  if (!valid_name(name)) {
    throw Error(std::string(table) + "." + name + ": names are letters and digits, starting with a letter");
  }
  if (parse_input(source) || std::find(seen.begin(), seen.end(), source) != seen.end()) {
    throw Error(std::string(table) + "." + name + ": its source \"" + source +
                "\" is already taken; touch sticks and buttons share the Touch<Name> names");
  }
  seen.push_back(source);
}

void check_counts(std::size_t sticks, std::size_t buttons) {
  if (sticks > kMaxTouchSticks) {
    throw Error("config.touchSticks: at most kMaxTouchSticks (" + std::to_string(kMaxTouchSticks) +
                ") sticks, got " + std::to_string(sticks));
  }
  if (buttons > kMaxTouchButtons) {
    throw Error("config.touchButtons: at most kMaxTouchButtons (" + std::to_string(kMaxTouchButtons) +
                ") buttons, got " + std::to_string(buttons));
  }
}

void set_stick_inputs(InputSet& held, std::size_t i, Vec2 v) {
  held.set(stick_code(i, 0), v.x < -kPressThreshold);
  held.set(stick_code(i, 1), v.x > kPressThreshold);
  held.set(stick_code(i, 2), v.y < -kPressThreshold);
  held.set(stick_code(i, 3), v.y > kPressThreshold);
  held.set(stick_code(i, 4));
}

// A finger counts this step unless it lifted after an earlier step (a tap shorter than a step counts).
bool counts(const Finger& f) { return !f.ended || f.began; }

void free_inputs(const std::vector<Finger>& free, float mid, InputSet& held) {
  for (const Finger& f : free) {
    if (!counts(f)) continue;
    for (const char* name : {"Touch", "MouseLeft", f.pos.x < mid ? "TouchLeft" : "TouchRight"}) {
      held.set(*parse_input(name));
    }
  }
}

void join(std::string& out, const std::string& item, const char* sep) { out += (out.empty() ? "" : sep) + item; }

}  // namespace

std::string pascal(std::string_view name) {
  std::string out(name);
  if (!out.empty()) out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
  return out;
}

void TouchPad::configure(std::vector<StickConfig> sticks, std::vector<ButtonConfig> buttons, bool active) {
  check_counts(sticks.size(), buttons.size());
  std::vector<std::string> seen;
  for (const StickConfig& s : sticks) check_name(s.name, "config.touchSticks", seen);
  for (const ButtonConfig& b : buttons) check_name(b.name, "config.touchButtons", seen);
  sticks_ = std::move(sticks);
  buttons_ = std::move(buttons);
  stick_states_.assign(sticks_.size(), {});
  button_states_.assign(buttons_.size(), {});
  ignored_.clear();
  active_ = active;
}

std::optional<InputCode> TouchPad::input(std::string_view name) const {
  if (const auto fixed = parse_input(name)) return fixed;
  for (std::size_t i = 0; i < sticks_.size(); ++i) {
    for (std::size_t k = 0; k < kStickInputs; ++k) {
      if (name == source_of(sticks_[i].name) + std::string(kStickSuffix[k])) return stick_code(i, k);
    }
  }
  for (std::size_t j = 0; j < buttons_.size(); ++j) {
    if (name == source_of(buttons_[j].name)) return button_code(j);
  }
  return std::nullopt;
}

std::optional<std::size_t> TouchPad::axis(std::string_view name) const {
  for (std::size_t i = 0; i < sticks_.size(); ++i) {
    const std::string base = source_of(sticks_[i].name);
    if (name == base + "X") return 2 * i;
    if (name == base + "Y") return 2 * i + 1;
  }
  return std::nullopt;
}

std::string TouchPad::input_names() const {
  std::string out;
  for (const StickConfig& s : sticks_) {
    for (std::string_view k : kStickSuffix) join(out, source_of(s.name) + std::string(k), " ");
  }
  for (const ButtonConfig& b : buttons_) join(out, source_of(b.name), " ");
  return out;
}

std::string TouchPad::axis_names() const {
  std::string out;
  for (const StickConfig& s : sticks_) join(out, source_of(s.name) + "X, " + source_of(s.name) + "Y", ", ");
  return out;
}

TouchFrame TouchPad::update(const std::vector<Finger>& fingers) {
  TouchFrame out;
  drop_missing(fingers);
  for (const Finger& f : fingers) {
    if (ignored(f)) continue;
    if (f.began && active_) capture(f);
    if (!owned(f, out)) out.free.push_back(f);
    if (f.ended) let_go(f.id);
  }
  free_inputs(out.free, screen_.view.x * 0.5f, out.held[0]);
  return out;
}

bool TouchPad::ignored(const Finger& f) {
  const auto it = std::find(ignored_.begin(), ignored_.end(), f.id);
  if (it == ignored_.end()) return false;
  if (f.ended) ignored_.erase(it);
  return true;
}

// Buttons first (one may sit inside a stick's zone), then sticks in order.
void TouchPad::capture(const Finger& f) {
  for (std::size_t j = 0; j < buttons_.size(); ++j) {
    if (!button_states_[j].finger && here(buttons_[j].scenes) && button_hit(buttons_[j], screen_, f.pos)) {
      button_states_[j].finger = f.id;
      return;
    }
  }
  for (std::size_t i = 0; i < sticks_.size(); ++i) {
    if (!stick_states_[i].finger && here(sticks_[i].scenes) && contains(stick_zone(sticks_[i], screen_), f.pos)) {
      const Vec2 center = stick_center(sticks_[i], screen_, f.pos);
      stick_states_[i] = {f.id, center, center, {}};
      return;
    }
  }
}

bool TouchPad::owned(const Finger& f, TouchFrame& out) {
  for (std::size_t j = 0; j < buttons_.size(); ++j) {
    if (button_states_[j].finger != f.id) continue;
    if (counts(f)) out.held[buttons_[j].player].set(button_code(j));
    return true;
  }
  for (std::size_t i = 0; i < sticks_.size(); ++i) {
    if (stick_states_[i].finger != f.id) continue;
    if (counts(f)) drive_stick(i, f, out);
    return true;
  }
  return false;
}

void TouchPad::drive_stick(std::size_t i, const Finger& f, TouchFrame& out) {
  const StickConfig& cfg = sticks_[i];
  StickState& st = stick_states_[i];
  Vec2 offset = f.pos - st.center;
  const float len = std::hypot(offset.x, offset.y);
  if (len > cfg.radius) offset = offset * (cfg.radius / len);
  st.knob = st.center + offset;
  st.value = stick_value(offset, cfg);
  out.analog[cfg.player][2 * i] = st.value.x;
  out.analog[cfg.player][2 * i + 1] = st.value.y;
  set_stick_inputs(out.held[cfg.player], i, st.value);
}

void TouchPad::let_go(std::uint64_t id) {
  for (StickState& st : stick_states_) {
    if (st.finger == id) st = {std::nullopt, st.center, st.center, {}};
  }
  for (ButtonState& b : button_states_) {
    if (b.finger == id) b.finger.reset();
  }
}

// A finger that vanished without lifting (the device dropped it) lets go too.
void TouchPad::drop_missing(const std::vector<Finger>& fingers) {
  const auto present = [&fingers](std::uint64_t id) {
    return std::any_of(fingers.begin(), fingers.end(), [id](const Finger& f) { return f.id == id; });
  };
  for (StickState& st : stick_states_) {
    if (st.finger && !present(*st.finger)) let_go(*st.finger);
  }
  for (ButtonState& b : button_states_) {
    if (b.finger && !present(*b.finger)) b.finger.reset();
  }
  std::erase_if(ignored_, [&present](std::uint64_t id) { return !present(id); });
}

void TouchPad::enter_scene(std::string scene) {
  release_all();
  scene_ = std::move(scene);
}

bool TouchPad::here(const std::vector<std::string>& scenes) const {
  return scenes.empty() || std::find(scenes.begin(), scenes.end(), scene_) != scenes.end();
}

void TouchPad::release_all() {
  std::vector<std::uint64_t> held;
  for (const StickState& st : stick_states_) if (st.finger) held.push_back(*st.finger);
  for (const ButtonState& b : button_states_) if (b.finger) held.push_back(*b.finger);
  for (std::uint64_t id : held) let_go(id);
  ignored_.insert(ignored_.end(), held.begin(), held.end());
}

}  // namespace alpha::core
