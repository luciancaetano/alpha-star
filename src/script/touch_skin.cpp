#include "script/touch_skin.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <charconv>
#include <vector>

#include "core/error.hpp"
#include "core/suggest.hpp"
#include "script/spec_fields.hpp"
#include "script/touch_config.hpp"

namespace alpha::script {

namespace {

constexpr std::array<const char*, 6> kStickSkinFields{"base", "knob", "baseSize", "knobSize", "pressed", "tint"};
constexpr std::array<const char*, 4> kButtonSkinFields{"up", "down", "size", "tint"};
constexpr std::array<const char*, 2> kPressedFields{"knob", "base"};

template <typename Map>
std::vector<std::string> asset_names(const Map& table) {
  std::vector<std::string> out;
  for (const auto& entry : table) out.push_back(entry.first);
  return out;
}

[[noreturn]] void missing(const std::string& what, const std::string& name, const char* kind,
                          const std::vector<std::string>& have) {
  const auto near = core::closest(name, have);
  throw core::Error(what + ": image \"" + name + "\" not in " + kind +
                    (near ? "; did you mean \"" + *near + "\"?" : "; have: " + core::join(have, " ")));
}

core::Picture sheet_frame(const ScriptState& s, const std::string& name, std::size_t colon, const std::string& what) {
  const std::string sheet_name = name.substr(0, colon);
  const auto it = s.sheets.find(sheet_name);
  if (it == s.sheets.end()) missing(what, sheet_name, "assets.sheets", asset_names(s.sheets));
  const core::Sheet& sheet = *it->second;
  int index = -1;
  const std::string frame = name.substr(colon + 1);
  const auto [end, err] = std::from_chars(frame.data(), frame.data() + frame.size(), index);
  if (err != std::errc{} || end != frame.data() + frame.size() || index < 0 || index >= sheet.frame_count()) {
    throw core::Error(what + ": \"" + name + "\" names frame " + frame + "; sheet \"" + sheet_name + "\" has frames 0.." +
                      std::to_string(sheet.frame_count() - 1));
  }
  return {sheet.picture.image, sheet.frame(index)};
}

core::Picture picture_at(JSContext* ctx, const ScriptState& s, JSValueConst obj, const char* name, const std::string& what) {
  const std::string path = what + "." + name;
  if (!has(ctx, obj, name)) throw core::Error(path + " is required (an assets.images name or \"sheet:frame\")");
  return skin_picture(s, string_arg(ctx, field(ctx, obj, name).get(), path), path);
}

std::optional<core::Picture> picture_or(JSContext* ctx, const ScriptState& s, JSValueConst obj, const char* name,
                                        const std::string& what) {
  if (!has(ctx, obj, name)) return std::nullopt;
  return picture_at(ctx, s, obj, name, what);
}

core::Color tint_of(JSContext* ctx, JSValueConst v, const std::string& what) {
  if (!has(ctx, v, "tint")) return {255, 255, 255, 255};
  return color_named(ctx, field(ctx, v, "tint").get(), what + ".tint");
}

}  // namespace

core::Picture skin_picture(const ScriptState& s, const std::string& name, const std::string& what) {
  if (const auto colon = name.find(':'); colon != std::string::npos) return sheet_frame(s, name, colon, what);
  const auto it = s.images.find(name);
  if (it == s.images.end()) missing(what, name, "assets.images", asset_names(s.images));
  return it->second;
}

core::StickSkin stick_skin(JSContext* ctx, const ScriptState& s, JSValueConst v, const std::string& what) {
  only_fields(ctx, v, what, kStickSkinFields);
  core::StickSkin skin;
  skin.base = picture_at(ctx, s, v, "base", what);
  skin.knob = picture_at(ctx, s, v, "knob", what);
  skin.base_size = number_in(ctx, v, "baseSize", what + ".baseSize", 0, 10000, 0.0f);
  skin.knob_size = number_in(ctx, v, "knobSize", what + ".knobSize", 0, 10000, 0.0f);
  skin.tint = tint_of(ctx, v, what);
  if (!has(ctx, v, "pressed")) return skin;
  const Value pressed = field(ctx, v, "pressed");
  only_fields(ctx, pressed.get(), what + ".pressed", kPressedFields);
  skin.base_pressed = picture_or(ctx, s, pressed.get(), "base", what + ".pressed");
  skin.knob_pressed = picture_or(ctx, s, pressed.get(), "knob", what + ".pressed");
  return skin;
}

core::ButtonSkin button_skin(JSContext* ctx, const ScriptState& s, JSValueConst v, const std::string& what) {
  only_fields(ctx, v, what, kButtonSkinFields);
  core::ButtonSkin skin;
  skin.up = picture_at(ctx, s, v, "up", what);
  skin.down = picture_or(ctx, s, v, "down", what);
  skin.size = number_in(ctx, v, "size", what + ".size", 0, 10000, 0.0f);
  skin.tint = tint_of(ctx, v, what);
  return skin;
}

}  // namespace alpha::script
