#include "script/asset_bindings.hpp"

#include <array>
#include <memory>

#include "core/error.hpp"
#include "core/index_of.hpp"
#include "platform/assets.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/synth_parser.hpp"

namespace alpha::script {

namespace {

std::string asset_path(ScriptState& s, const std::string& file) {
  return (s.asset_dir / file).string();
}

template <typename Map>
std::string names(const Map& map) {
  const std::string out = core::keys_of(map, ", ");
  return out.empty() ? "(none)" : out;
}

core::SoundPtr synth(JSContext* ctx, JSValueConst spec, const std::string& what) {
  const auto tones = parse_synth(ctx, spec, what);
  return std::make_shared<const std::vector<float>>(core::synthesize(tones, core::Mixer::kSampleRate));
}

// argv[first..first+2] = volume, pitch, loop
core::PlayOptions play_options(JSContext* ctx, JSValueConst* argv, int first) {
  return {static_cast<float>(number_arg(ctx, argv[first], "audio volume")),
          static_cast<float>(number_arg(ctx, argv[first + 1], "audio pitch")),
          JS_ToBool(ctx, argv[first + 2]) == 1};
}

core::SoundPtr decode_sound(const std::string& path) {
  return std::make_shared<const std::vector<float>>(platform::load_wav(path));
}

constexpr char kSounds[] = "sounds";

// Native loading file asset assets.<Kind>.<name> (argv = name, file) into s.*Table.
template <auto Table, auto Decode, const char* Kind>
JSValue load_file(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string kind = Kind;
  const std::string name = string_arg(ctx, argv[0], kind + " name");
  const std::string file = string_arg(ctx, argv[1], "assets." + kind + "." + name);
  (s.*Table)[name] = Decode(asset_path(s, file));
  return JS_UNDEFINED;
}

JSValue synth_sound(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "sound name");
  s.sounds[name] = synth(ctx, argv[1], "assets.sounds." + name);
  return JS_UNDEFINED;
}

JSValue play_sound(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "sound");
  const auto it = s.sounds.find(name);
  if (it == s.sounds.end()) {
    throw core::Error("audio.play(\"" + name + "\"): unknown sound; assets.sounds has: " + names(s.sounds));
  }
  return JS_NewUint32(ctx, s.mixer.play(it->second, play_options(ctx, argv, 1)));
}

JSValue play_tone(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewUint32(ctx, s.mixer.play(synth(ctx, argv[0], "audio.tone"), play_options(ctx, argv, 1)));
}

JSValue stop_voice(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  s.mixer.stop(static_cast<core::VoiceId>(number_arg(ctx, argv[0], "voice id")));
  return JS_UNDEFINED;
}

JSValue voice_playing(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewBool(ctx, s.mixer.playing(static_cast<core::VoiceId>(number_arg(ctx, argv[0], "voice id"))));
}

JSValue stop_all(JSContext*, ScriptState& s, JSValueConst*) {
  s.mixer.stop_all();
  return JS_UNDEFINED;
}

JSValue set_volume(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const double v = number_arg(ctx, argv[0], "audio.volume");
  if (v < 0.0 || v > 1.0) throw core::Error("audio.volume must be in [0, 1]");
  s.mixer.set_volume(static_cast<float>(v));
  return JS_UNDEFINED;
}

constexpr std::array<Native, 8> kNatives{{
    {"loadSound", guarded<load_file<&ScriptState::sounds, decode_sound, kSounds>>, 2},
    {"synthSound", guarded<synth_sound>, 2},   {"playSound", guarded<play_sound>, 4},
    {"playTone", guarded<play_tone>, 4},     {"stopVoice", guarded<stop_voice>, 1},
    {"voicePlaying", guarded<voice_playing>, 1}, {"stopAll", guarded<stop_all>, 0},
    {"setVolume", guarded<set_volume>, 1},
}};

}  // namespace

void install_asset_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
