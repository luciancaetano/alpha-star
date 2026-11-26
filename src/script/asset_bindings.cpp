#include "script/asset_bindings.hpp"

#include <array>
#include <memory>

#include "core/error.hpp"
#include "core/limits.hpp"
#include "core/shader.hpp"
#include "platform/music_stream.hpp"
#include "platform/shader_files.hpp"
#include "platform/vfs.hpp"
#include "script/audio_bindings.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#include "script/spec_fields.hpp"
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

// assets.shaders.<name>.uniforms: { name: default } in uniform block order.
std::vector<std::pair<std::string, float>> uniforms_arg(JSContext* ctx, JSValueConst v, const std::string& what) {
  std::vector<std::pair<std::string, float>> out;
  if (JS_IsUndefined(v)) return out;
  if (!JS_IsObject(v)) throw core::Error(what + " must be { name: default number }, got " + describe(ctx, v));
  const std::vector<std::string> names = keys(ctx, v);
  core::check_uniform_count(what, names.size());
  for (const std::string& key : names) {
    out.emplace_back(key, static_cast<float>(number_arg(ctx, field(ctx, v, key.c_str()).get(), what + "." + key)));
  }
  return out;
}

// loadShader(name, { file, uniforms? }): validated, its blobs compiled (dev) or read from the pack.
JSValue load_shader(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string name = string_arg(ctx, argv[0], "shader name");
  const std::string what = "assets.shaders." + name;
  if (!JS_IsObject(argv[1])) throw core::Error(what + " must be { file: \"x.frag\", uniforms?: { name: number } }, got " + describe(ctx, argv[1]));
  if (s.shaders.size() >= core::kMaxShaders) throw core::Error(what + ": at most " + std::to_string(core::kMaxShaders) + " shaders");
  core::Shader shader = core::make_shader(name, string_arg(ctx, field(ctx, argv[1], "file").get(), what + ".file"),
                                          uniforms_arg(ctx, field(ctx, argv[1], "uniforms").get(), what + ".uniforms"));
  shader.blobs = platform::load_shader_blobs(s.asset_dir, shader.file, what);
  s.shaders[name] = std::make_shared<const core::Shader>(std::move(shader));
  return JS_UNDEFINED;
}

constexpr std::array<Native, 4> kNatives{{
    {"loadShader", guarded<load_shader>, 2},
    {"loadSound", guarded<load_sound>, 3},
    {"synthSound", guarded<synth_sound>, 3},
    {"loadMusic", guarded<load_music>, 2},
}};

}  // namespace

void install_asset_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
