#include "script/touch_config.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#include "core/error.hpp"
#include "core/limits.hpp"
#include "script/spec_fields.hpp"
#include "script/touch_skin.hpp"

namespace alpha::script {

namespace {

constexpr std::array<const char*, 12> kStickFields{"zone",    "mode",    "anchor", "radius", "deadzone", "axes",
                                                   "snap",    "visible", "opacity", "skin", "player", "scenes"};
constexpr std::array<const char*, 9> kButtonFields{"pos",     "radius",  "hitSlop", "label",
                                                   "visible", "opacity", "skin",    "player", "scenes"};

// obj[name] as one of `options` (by name), else `fallback`.
template <typename E, std::size_t N>
E pick(JSContext* ctx, JSValueConst obj, const char* name, const std::string& what,
       const std::array<std::pair<const char*, E>, N>& options, E fallback) {
  if (!has(ctx, obj, name)) return fallback;
  const std::string text = string_arg(ctx, field(ctx, obj, name).get(), what);
  std::string allowed;
  for (const auto& [option, value] : options) {
    if (text == option) return value;
    allowed += std::string(allowed.empty() ? "\"" : "|\"") + option + "\"";
  }
  throw core::Error(what + ": expected " + allowed + ", got \"" + text + "\"");
}

bool all_fractions(const std::vector<double>& n) {
  return std::all_of(n.begin(), n.end(), [](double x) { return x >= 0.0 && x <= 1.0; });
}

// [x, y] with both in 0..1.
core::Vec2 fraction_pair(JSContext* ctx, JSValueConst v, const std::string& what) {
  const std::vector<double> n = numbers_arg(ctx, v, what);
  const bool ok = n.size() == 2 && all_fractions(n);
  if (!ok) throw core::Error(what + ": expected [x, y] in 0..1 (fractions of ctx.safeArea), got " + describe(ctx, v));
  return {static_cast<float>(n[0]), static_cast<float>(n[1])};
}

// [x, y, w, h] in 0..1, non-empty and inside the safe area.
std::optional<core::Box> zone_rect(const std::vector<double>& n) {
  if (n.size() != 4 || !all_fractions(n) || n[2] <= 0.0 || n[3] <= 0.0) return std::nullopt;
  if (n[0] + n[2] > 1.0 || n[1] + n[3] > 1.0) return std::nullopt;
  const auto f = [](double x) { return static_cast<float>(x); };
  return core::Box{{f(n[0]), f(n[1])}, {f(n[0] + n[2]), f(n[1] + n[3])}};
}

std::optional<core::Box> zone_side(const std::string& side) {
  if (side == "left") return core::Box{{0.0f, 0.0f}, {0.5f, 1.0f}};
  if (side == "right") return core::Box{{0.5f, 0.0f}, {1.0f, 1.0f}};
  return std::nullopt;
}

core::Box zone_of(JSContext* ctx, JSValueConst v, const std::string& what) {
  std::optional<core::Box> zone;
  if (JS_IsString(v)) zone = zone_side(string_arg(ctx, v, what));
  else if (JS_IsArray(v)) zone = zone_rect(numbers_arg(ctx, v, what));
  if (zone) return *zone;
  const std::string shown = JS_IsString(v) ? "\"" + describe(ctx, v) + "\"" : describe(ctx, v);
  throw core::Error(what + ": expected \"left\"|\"right\"|[x,y,w,h] in 0..1, got " + shown);
}

std::size_t player_of(JSContext* ctx, JSValueConst obj, const std::string& what) {
  const float p = number_in(ctx, obj, "player", what + ".player", 1, static_cast<double>(core::kMaxPlayers), 1.0f);
  if (p != static_cast<float>(static_cast<int>(p))) throw core::Error(what + ".player must be an integer 1..4");
  return static_cast<std::size_t>(p) - 1;
}

// obj.scenes: names of scenes (the runtime checks them against game.scenes).
std::vector<std::string> scenes_of(JSContext* ctx, JSValueConst obj, const std::string& what) {
  if (!has(ctx, obj, "scenes")) return {};
  return strings_arg(ctx, field(ctx, obj, "scenes").get(), what + ".scenes");
}

int snap_of(JSContext* ctx, JSValueConst obj, const std::string& what) {
  const float snap = number_in(ctx, obj, "snap", what + ".snap", 0, 8, 0.0f);
  if (snap != 0.0f && snap != 4.0f && snap != 8.0f) throw core::Error(what + ".snap must be 0, 4 or 8");
  return static_cast<int>(snap);
}

core::StickConfig stick_of(JSContext* ctx, const ScriptState& s, const std::string& name, JSValueConst v) {
  const std::string what = "config.touchSticks." + name;
  only_fields(ctx, v, what, kStickFields);
  core::StickConfig c;
  c.name = name;
  if (has(ctx, v, "zone")) c.zone = zone_of(ctx, field(ctx, v, "zone").get(), what + ".zone");
  c.mode = pick(ctx, v, "mode", what + ".mode",
                std::array{std::pair{"floating", core::StickMode::Floating}, std::pair{"fixed", core::StickMode::Fixed}},
                core::StickMode::Floating);
  if (has(ctx, v, "anchor")) c.anchor = fraction_pair(ctx, field(ctx, v, "anchor").get(), what + ".anchor");
  c.radius = number_in(ctx, v, "radius", what + ".radius", 1, 10000, 64.0f);
  c.deadzone = number_in(ctx, v, "deadzone", what + ".deadzone", 0, 0.9, 0.15f);
  c.axes = pick(ctx, v, "axes", what + ".axes",
                std::array{std::pair{"both", core::StickAxes::Both}, std::pair{"x", core::StickAxes::X},
                           std::pair{"y", core::StickAxes::Y}},
                core::StickAxes::Both);
  c.snap = snap_of(ctx, v, what);
  c.always_visible = pick(ctx, v, "visible", what + ".visible",
                          std::array{std::pair{"always", true}, std::pair{"touching", false}},
                          c.mode == core::StickMode::Fixed);
  c.opacity = number_in(ctx, v, "opacity", what + ".opacity", 0, 1, 0.6f);
  if (has(ctx, v, "skin")) c.skin = stick_skin(ctx, s, field(ctx, v, "skin").get(), what + ".skin");
  c.player = player_of(ctx, v, what);
  c.scenes = scenes_of(ctx, v, what);
  return c;
}

core::ButtonConfig button_of(JSContext* ctx, const ScriptState& s, const std::string& name, JSValueConst v) {
  const std::string what = "config.touchButtons." + name;
  only_fields(ctx, v, what, kButtonFields);
  core::ButtonConfig c;
  c.name = name;
  if (!has(ctx, v, "pos")) throw core::Error(what + ".pos is required: [x, y] in 0..1 (fractions of ctx.safeArea)");
  c.pos = fraction_pair(ctx, field(ctx, v, "pos").get(), what + ".pos");
  c.radius = number_in(ctx, v, "radius", what + ".radius", 1, 10000, 40.0f);
  c.hit_slop = number_in(ctx, v, "hitSlop", what + ".hitSlop", 0, 10000, 8.0f);
  if (has(ctx, v, "label")) c.label = text_arg(ctx, field(ctx, v, "label").get(), what + ".label");
  c.visible = pick(ctx, v, "visible", what + ".visible",
                   std::array{std::pair{"always", true}, std::pair{"never", false}}, true);
  c.opacity = number_in(ctx, v, "opacity", what + ".opacity", 0, 1, 0.6f);
  if (has(ctx, v, "skin")) c.skin = button_skin(ctx, s, field(ctx, v, "skin").get(), what + ".skin");
  c.player = player_of(ctx, v, what);
  c.scenes = scenes_of(ctx, v, what);
  return c;
}

// Entries of a name -> spec object, capped before any is parsed.
std::vector<std::string> entries(JSContext* ctx, JSValueConst table, const std::string& what, std::size_t cap,
                                 const char* cap_name) {
  if (JS_IsUndefined(table)) return {};
  if (!JS_IsObject(table) || JS_IsArray(table)) throw core::Error(what + " must be an object of name -> spec");
  std::vector<std::string> names = keys(ctx, table);
  if (names.size() > cap) {
    throw core::Error(what + ": at most " + cap_name + " (" + std::to_string(cap) + "), got " +
                      std::to_string(names.size()));
  }
  return names;
}

// Every entry of a name -> spec object parsed by `parse`, capped before any is parsed.
template <typename Parse>
auto parse_table(JSContext* ctx, const ScriptState& s, JSValueConst table, const std::string& what, std::size_t cap,
                 const char* cap_name, Parse parse) {
  std::vector<decltype(parse(ctx, s, std::string(), table))> out;
  for (const std::string& name : entries(ctx, table, what, cap, cap_name)) {
    out.push_back(parse(ctx, s, name, field(ctx, table, name.c_str()).get()));
  }
  return out;
}

void check_overlaps(const std::vector<core::ButtonConfig>& buttons, core::Vec2 size) {
  const core::TouchScreen screen{{{0.0f, 0.0f}, size}, size};
  for (std::size_t a = 0; a < buttons.size(); ++a) {
    for (std::size_t b = a + 1; b < buttons.size(); ++b) {
      if (!core::buttons_overlap(buttons[a], buttons[b], screen)) continue;
      throw core::Error("config.touchButtons." + buttons[a].name + " and ." + buttons[b].name +
                        " overlap at config.size (radius + hitSlop): a finger between them is ambiguous; move them apart");
    }
  }
}

}  // namespace

void only_fields(JSContext* ctx, JSValueConst obj, const std::string& what, std::span<const char* const> fields) {
  if (!JS_IsObject(obj) || JS_IsArray(obj)) throw core::Error(what + " must be an object");
  for (const std::string& key : keys(ctx, obj)) {
    if (std::find(fields.begin(), fields.end(), key) != fields.end()) continue;
    throw core::Error(what + ": unknown field \"" + key + "\"; fields: " + core::join(fields, ", "));
  }
}

float number_in(JSContext* ctx, JSValueConst obj, const char* name, const std::string& what, double lo, double hi,
                float fallback) {
  return has(ctx, obj, name) ? ranged(ctx, field(ctx, obj, name).get(), what, lo, hi) : fallback;
}

std::vector<core::StickConfig> touch_sticks(JSContext* ctx, const ScriptState& s, JSValueConst sticks) {
  return parse_table(ctx, s, sticks, "config.touchSticks", core::kMaxTouchSticks, "kMaxTouchSticks", stick_of);
}

std::vector<core::ButtonConfig> touch_buttons(JSContext* ctx, const ScriptState& s, JSValueConst buttons) {
  auto out = parse_table(ctx, s, buttons, "config.touchButtons", core::kMaxTouchButtons, "kMaxTouchButtons", button_of);
  check_overlaps(out, s.design_size);
  return out;
}

}  // namespace alpha::script
