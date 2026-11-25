#include "script/synth_parser.hpp"

#include <array>
#include <utility>

#include "core/error.hpp"
#include "script/js_value.hpp"

namespace alpha::script {

namespace {

constexpr std::array<std::pair<const char*, core::Wave>, 5> kWaves{{
    {"sine", core::Wave::Sine},
    {"square", core::Wave::Square},
    {"triangle", core::Wave::Triangle},
    {"saw", core::Wave::Saw},
    {"noise", core::Wave::Noise},
}};

constexpr float kMaxDuration = 10.0f;
constexpr float kMaxFreq = 20000.0f;

core::Wave wave(JSContext* ctx, const Value& note, const std::string& what) {
  const Value v = note.prop("wave");
  const std::string name = string_arg(ctx, v.get(), what + ".wave");
  for (const auto& [key, w] : kWaves) {
    if (name == key) return w;
  }
  throw core::Error(what + ".wave must be sine, square, triangle, saw or noise, got \"" + name + "\"");
}

float in_range(float x, const std::string& what, float min, float max) {
  if (x < min || x > max) {
    throw core::Error(what + " must be in [" + std::to_string(min) + ", " + std::to_string(max) +
                      "], got " + std::to_string(x));
  }
  return x;
}

float ranged(JSContext* ctx, JSValueConst v, const std::string& what, float min, float max) {
  return in_range(static_cast<float>(number_arg(ctx, v, what)), what, min, max);
}

// Optional field in [0, max]; `fallback` when absent.
float optional(JSContext* ctx, const Value& note, const char* name, const std::string& what,
               std::pair<float, float> fallback_max) {
  const Value v = note.prop(name);
  if (JS_IsUndefined(v.get())) return fallback_max.first;
  return ranged(ctx, v.get(), what + "." + name, 0.0f, fallback_max.second);
}

core::Tone tone(JSContext* ctx, const Value& note, const std::string& what) {
  if (!JS_IsObject(note.get())) throw core::Error(what + " must be { wave, freq, duration, ... }");
  core::Tone t;
  t.wave = wave(ctx, note, what);
  const Value freq = note.prop("freq");
  if (JS_IsArray(freq.get())) {
    const core::Vec2 slide = vec2_arg(ctx, freq.get(), what + ".freq");
    t.freq_start = in_range(slide.x, what + ".freq[0]", 1.0f, kMaxFreq);
    t.freq_end = in_range(slide.y, what + ".freq[1]", 1.0f, kMaxFreq);
  } else {
    t.freq_start = t.freq_end = ranged(ctx, freq.get(), what + ".freq", 1.0f, kMaxFreq);
  }
  const Value duration = note.prop("duration");
  t.duration = ranged(ctx, duration.get(), what + ".duration", 0.001f, kMaxDuration);
  t.volume = optional(ctx, note, "volume", what, {t.volume, 1.0f});
  t.attack = optional(ctx, note, "attack", what, {t.attack, kMaxDuration});
  t.release = optional(ctx, note, "release", what, {t.release, kMaxDuration});
  return t;
}

}  // namespace

std::vector<core::Tone> parse_synth(JSContext* ctx, JSValueConst spec, const std::string& what) {
  const Value copy(ctx, JS_DupValue(ctx, spec));
  if (!JS_IsArray(spec)) return {tone(ctx, copy, what)};
  int64_t length = 0;
  JS_GetLength(ctx, spec, &length);
  if (length == 0) throw core::Error(what + " must contain at least one note");
  std::vector<core::Tone> tones;
  for (int64_t i = 0; i < length; ++i) {
    tones.push_back(tone(ctx, copy.at(static_cast<std::uint32_t>(i)), what + "[" + std::to_string(i) + "]"));
  }
  return tones;
}

}  // namespace alpha::script
