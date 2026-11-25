#include "core/postfx.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "core/error.hpp"

namespace alpha::core {

namespace {

struct Param {
  const char* name;
  float lo;
  float hi;
  float Effect::*field;
};

struct Rule {
  EffectKind kind;
  const char* name;
  std::vector<Param> params;
  bool color = false;
  Effect defaults;
};

const std::array<Rule, 5>& rules() {
  static const std::array<Rule, 5> table{{
      {EffectKind::Bloom, "bloom", {{"strength", 0, 2, &Effect::strength}, {"radius", 1, 16, &Effect::radius}},
       false, {EffectKind::Bloom, 0.6f, 4.0f}},
      {EffectKind::Crt, "crt", {{"strength", 0, 1, &Effect::strength}}, false, {EffectKind::Crt, 0.5f}},
      {EffectKind::Pixelate, "pixelate", {{"size", 1, 64, &Effect::size}}, false, {EffectKind::Pixelate}},
      {EffectKind::Tint, "tint", {{"strength", 0, 1, &Effect::strength}}, true, {EffectKind::Tint, 1.0f}},
      {EffectKind::Vignette, "vignette", {{"strength", 0, 1, &Effect::strength}, {"radius", 0, 1, &Effect::radius}},
       false, {EffectKind::Vignette, 0.5f, 0.5f}},
  }};
  return table;
}

const Rule* rule_of(std::string_view name) {
  for (const Rule& r : rules()) {
    if (name == r.name) return &r;
  }
  return nullptr;
}

std::string param_names(const Rule& r) {
  std::string out;
  for (const Param& p : r.params) out += (out.empty() ? "" : " ") + std::string(p.name);
  return r.color ? out + " color" : out;
}

void apply(const Rule& r, const std::string& key, float value, Effect& out) {
  const std::string where = "postfx \"" + std::string(r.name) + "\"";
  const auto p = std::find_if(r.params.begin(), r.params.end(), [&](const Param& x) { return key == x.name; });
  if (p == r.params.end()) throw Error(where + ": unknown param \"" + key + "\"; have: " + param_names(r));
  if (!(value >= p->lo && value <= p->hi)) {
    throw Error(where + "." + key + " must be " + std::to_string(p->lo) + ".." + std::to_string(p->hi) + ", got " +
                std::to_string(value));
  }
  out.*(p->field) = value;
}

}  // namespace

Effect make_effect(std::string_view name, const EffectNumbers& numbers, std::optional<Color> color) {
  const Rule* r = rule_of(name);
  if (!r) throw Error("postfx: unknown effect \"" + std::string(name) + "\"; have: " + effect_names());
  Effect out = r->defaults;
  for (const auto& [key, value] : numbers) apply(*r, key, value, out);
  if (color && !r->color) throw Error("postfx \"" + std::string(name) + "\": unknown param \"color\"; have: " + param_names(*r));
  if (color) out.color = *color;
  if (r->kind == EffectKind::Pixelate) out.size = std::round(out.size);
  return out;
}

std::string_view effect_name(EffectKind kind) {
  for (const Rule& r : rules()) {
    if (r.kind == kind) return r.name;
  }
  return "";
}

std::optional<EffectKind> effect_kind(std::string_view name) {
  const Rule* r = rule_of(name);
  return r ? std::optional<EffectKind>(r->kind) : std::nullopt;
}

std::string effect_names() {
  std::string out;
  for (const Rule& r : rules()) out += (out.empty() ? "" : " ") + std::string(r.name);
  return out;
}

bool needs_scene(const std::vector<Effect>& chain) {
  return std::any_of(chain.begin(), chain.end(),
                     [](const Effect& e) { return e.kind == EffectKind::Bloom || e.kind == EffectKind::Pixelate; });
}

void PostFx::put(const Effect& effect) {
  const auto it = std::find_if(chain_.begin(), chain_.end(), [&](const Effect& e) { return e.kind == effect.kind; });
  if (it != chain_.end()) *it = effect;
  else chain_.push_back(effect);
}

bool PostFx::remove(EffectKind kind) {
  return std::erase_if(chain_, [kind](const Effect& e) { return e.kind == kind; }) > 0;
}

}  // namespace alpha::core
