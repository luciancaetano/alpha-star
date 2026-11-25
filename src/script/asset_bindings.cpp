#include "script/asset_bindings.hpp"

#include <array>
#include <memory>

#include "core/error.hpp"
#include "platform/music_stream.hpp"
#include "platform/vfs.hpp"
#include "script/audio_bindings.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/synth_parser.hpp"

namespace alpha::script {

namespace {

std::string asset_path(ScriptState& s, const std::string& file) {
  return platform::vfs::in_pack(s.asset_dir, file).string();
}

// loadSound(name, file, bus): decoded whole at boot (.wav, .ogg, .mp3).
JSValue load_sound(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "sound name");
  const std::string file = string_arg(ctx, argv[1], "assets.sounds." + name);
  const core::BusId bus = bus_arg(ctx, s, argv[2], "assets.sounds." + name + ".bus");
  s.sounds[name] = {std::make_shared<const std::vector<float>>(platform::load_sound(asset_path(s, file))), bus};
  return JS_UNDEFINED;
}

// synthSound(name, spec, bus)
JSValue synth_sound(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "sound name");
  const core::BusId bus = bus_arg(ctx, s, argv[2], "assets.sounds." + name + ".bus");
  s.sounds[name] = {synth_samples(ctx, argv[1], "assets.sounds." + name), bus};
  return JS_UNDEFINED;
}

// loadMusic(name, file): opened once to check it decodes, then streamed when played.
JSValue load_music(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "music name");
  const std::string path = asset_path(s, string_arg(ctx, argv[1], "assets.music." + name));
  platform::open_music(path);
  s.music[name] = path;
  return JS_UNDEFINED;
}

constexpr std::array<Native, 3> kNatives{{
    {"loadSound", guarded<load_sound>, 3},
    {"synthSound", guarded<synth_sound>, 3},
    {"loadMusic", guarded<load_music>, 2},
}};

}  // namespace

void install_asset_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
