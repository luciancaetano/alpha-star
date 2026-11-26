#include "script/vm.hpp"

#include "script/asset_bindings.hpp"
#include "script/audio_bindings.hpp"
#include "script/prefs_bindings.hpp"
#include "script/settings_bindings.hpp"
#include "platform/vfs.hpp"
#include "script/bundle_map.hpp"
#include "script/bytecode.hpp"
#include "script/camera_bindings.hpp"
#include "script/effect_bindings.hpp"
#include "script/joint_bindings.hpp"
#include "script/physics_bindings.hpp"
#include "script/postfx_bindings.hpp"
#include "script/embedded_js.hpp"
#include "script/input_bindings.hpp"
#include "script/instance_bindings.hpp"
#include "script/map_bindings.hpp"
#include "script/net_bindings.hpp"
#include "script/sprite_bindings.hpp"
#include "script/system_bindings.hpp"
#include "script/ui_bindings.hpp"
#include "script/world_bindings.hpp"

namespace alpha::script {

Vm::Vm(SavePathFor save_path_for, core::LogRing& log)
    : instances(std::move(save_path_for), log), engine(std::make_unique<Engine>()) {
  JSContext* ctx = engine->ctx();
  JS_SetContextOpaque(ctx, &instances);
  engine->add_builtin("alpha", embedded::kAlpha);
#if ALPHA_DEBUG_TOOLS  // test runner only; importing it elsewhere lists the builtins that exist
  engine->add_builtin("alpha/test", embedded::kAlphaTest);
#endif
  engine->add_builtin("alpha:ldtk", embedded::kLdtk);
  engine->add_builtin("alpha:levels", embedded::kLevels);
  engine->add_builtin("alpha:tween", embedded::kTween);
  engine->add_builtin("alpha:flow", embedded::kFlow);
  engine->add_builtin("alpha:physics", embedded::kPhysics);
  engine->add_builtin("alpha:audio", embedded::kAudio);
  engine->add_builtin("alpha:prefs", embedded::kPrefs);
  engine->add_builtin("alpha:i18n", embedded::kI18n);
  engine->add_builtin("alpha:net", embedded::kNet);
  engine->add_builtin("alpha:runtime", embedded::kRuntime);
  const Value global(ctx, JS_GetGlobalObject(ctx));
  Value natives(ctx, JS_NewObject(ctx));
  install_world_bindings(ctx, natives.get());
  install_system_bindings(ctx, natives.get());
  install_input_bindings(ctx, natives.get());
  install_asset_bindings(ctx, natives.get());
  install_audio_bindings(ctx, natives.get());
  install_prefs_bindings(ctx, natives.get());
  install_settings_bindings(ctx, natives.get());
  install_instance_bindings(ctx, natives.get());
  install_ui_bindings(ctx, natives.get());
  install_sprite_bindings(ctx, natives.get());
  install_camera_bindings(ctx, natives.get());
  install_map_bindings(ctx, natives.get());
  install_effect_bindings(ctx, natives.get());
  install_postfx_bindings(ctx, natives.get());
  install_physics_bindings(ctx, natives.get());
  install_joint_bindings(ctx, natives.get());
  install_net_bindings(ctx, natives.get());
  JS_SetPropertyStr(ctx, global.get(), "__native", natives.release());
}

Value Vm::import_bundle(const std::string& path) {
  bundle_path = path;
  if (platform::vfs::lower_extension(path) == ".jsbc") {  // standalone pack: no source, no map
    source_map.reset();
    return import_bytecode_file(*engine, path);
  }
  source_map = load_bundle_map(engine->ctx(), path);
  return engine->import(path);
}

std::string Vm::located(const std::exception& e) const {
  return source_map ? core::rewrite_locations(e.what(), bundle_path, *source_map) : e.what();
}

Value Vm::runtime(const char* name) {
  if (!runtime_ns) runtime_ns.emplace(engine->import("alpha:runtime"));
  return runtime_ns->prop(name);
}

}  // namespace alpha::script
