#include "script/audio_bindings.hpp"

#include <array>
#include <memory>

#include "core/error.hpp"
#include "core/index_of.hpp"
#include "platform/music_stream.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/spec_fields.hpp"
#include "script/synth_parser.hpp"

namespace alpha::script {

namespace {

std::string list(const std::vector<std::string>& names) {
  std::string out;
  for (const auto& n : names) out += (out.empty() ? "" : ", ") + n;
  return out.empty() ? "(none)" : out;
}

// argv[first..first+3] = volume, pitch, loop, pan
core::PlayOptions play_options(JSContext* ctx, JSValueConst* argv, int first) {
  core::PlayOptions o;
  o.volume = ranged(ctx, argv[first], "audio volume", 0.0, 1.0);
  o.pitch = static_cast<float>(number_arg(ctx, argv[first + 1], "audio pitch"));
  if (!(o.pitch > 0.0f)) throw core::Error("audio pitch must be > 0");
  o.loop = JS_ToBool(ctx, argv[first + 2]) == 1;
  o.pan = ranged(ctx, argv[first + 3], "audio pan", -1.0, 1.0);
  return o;
}

JSValue play_sound(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "sound");
  const auto it = s.sounds.find(name);
  if (it == s.sounds.end()) {
    const std::string have = core::keys_of(s.sounds, ", ");
    throw core::Error("audio.play(\"" + name + "\"): unknown sound; assets.sounds has: " + (have.empty() ? "(none)" : have));
  }
  core::PlayOptions o = play_options(ctx, argv, 1);
  o.bus = it->second.bus;
  return JS_NewUint32(ctx, s.mixer.play(it->second.samples, o));
}

JSValue play_tone(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewUint32(ctx, s.mixer.play(synth_samples(ctx, argv[0], "audio.tone"), play_options(ctx, argv, 1)));
}

core::VoiceId voice_arg(JSContext* ctx, JSValueConst v) {
  return static_cast<core::VoiceId>(number_arg(ctx, v, "voice id"));
}

JSValue stop_voice(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.mixer.stop(voice_arg(ctx, argv[0]));
  return JS_UNDEFINED;
}

JSValue voice_playing(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewBool(ctx, s.mixer.playing(voice_arg(ctx, argv[0])));
}

JSValue stop_all(JSContext*, ScriptState& s, JSValueConst*) {
  s.mixer.stop_all();
  return JS_UNDEFINED;
}

JSValue add_bus(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.mixer.add_bus(string_arg(ctx, argv[0], "config.audio.buses entry"));
  return JS_UNDEFINED;
}

// busVolume(name, volume?): sets when volume is given; returns the volume.
JSValue bus_volume(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const core::BusId bus = bus_arg(ctx, s, argv[0], "audio.bus");
  if (!JS_IsUndefined(argv[1])) {
    ranged(ctx, argv[1], "audio.bus(...).volume", 0.0, 1.0);
    s.mixer.set_bus_volume(bus, number_arg(ctx, argv[1], "audio.bus(...).volume"));  // exact: read back as set
  }
  return JS_NewFloat64(ctx, s.mixer.bus_volume(bus));
}

JSValue buses(JSContext* ctx, ScriptState& s, JSValueConst*) {
  const auto names = s.mixer.bus_names();
  return new_strings(ctx, names);
}

// music(name, volume, loop, seconds, crossfade): fade in over seconds, or
// crossfade the current music out over the same seconds.
JSValue music(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "music");
  const auto it = s.music.find(name);
  if (it == s.music.end()) {
    const std::string have = core::keys_of(s.music, ", ");
    throw core::Error("audio.music(\"" + name + "\"): unknown music; assets.music has: " + (have.empty() ? "(none)" : have));
  }
  core::MusicOptions o;
  o.volume = ranged(ctx, argv[1], "music volume", 0.0, 1.0);
  o.loop = JS_ToBool(ctx, argv[2]) == 1;
  const float seconds = ranged(ctx, argv[3], "music fade seconds", 0.0, 60.0);
  if (JS_ToBool(ctx, argv[4]) == 1) {
    s.mixer.crossfade(name, platform::open_music(it->second), o, seconds);
  } else {
    o.fade_in = seconds;
    s.mixer.music(name, platform::open_music(it->second), o);
  }
  return JS_UNDEFINED;
}

JSValue stop_music(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.mixer.stop_music(ranged(ctx, argv[0], "stopMusic fadeOut seconds", 0.0, 60.0));
  return JS_UNDEFINED;
}

JSValue music_name(JSContext* ctx, ScriptState& s, JSValueConst*) {
  const std::string name = s.mixer.music_name();
  return name.empty() ? JS_NULL : JS_NewString(ctx, name.c_str());
}

constexpr std::array<Native, 11> kNatives{{
    {"playSound", guarded<play_sound>, 5},     {"playTone", guarded<play_tone>, 5},
    {"stopVoice", guarded<stop_voice>, 1},     {"voicePlaying", guarded<voice_playing>, 1},
    {"stopAll", guarded<stop_all>, 0},         {"addBus", guarded<add_bus>, 1},
    {"busVolume", guarded<bus_volume>, 2},     {"buses", guarded<buses>, 0},
    {"music", guarded<music>, 5},              {"stopMusic", guarded<stop_music>, 1},
    {"musicName", guarded<music_name>, 0},
}};

}  // namespace

core::BusId bus_arg(JSContext* ctx, ScriptState& s, JSValueConst v, const std::string& what) {
  const std::string name = string_arg(ctx, v, what);
  const auto bus = s.mixer.find_bus(name);
  if (!bus) throw core::Error(what + ": unknown bus \"" + name + "\"; buses: " + list(s.mixer.bus_names()) + " (add more in config.audio.buses)");
  return *bus;
}

core::SoundPtr synth_samples(JSContext* ctx, JSValueConst spec, const std::string& what) {
  const auto tones = parse_synth(ctx, spec, what);
  return std::make_shared<const std::vector<float>>(core::synthesize(tones, core::Mixer::kSampleRate));
}

void install_audio_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
