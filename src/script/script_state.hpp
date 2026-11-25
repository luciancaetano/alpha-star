#pragma once

#include <quickjs.h>

#include <filesystem>
#include <map>
#include <memory>
#include <string>

#include "core/image.hpp"
#include "core/input.hpp"
#include "core/mixer.hpp"
#include "core/random.hpp"
#include "core/tags.hpp"
#include "core/timers.hpp"
#include "ecs/world.hpp"

namespace alpha::script {

// Engine state the natives act on. One per Host; reached from JS via the
// context opaque pointer. The runtime replaces `world` on every scene enter.
struct ScriptState {
  core::TagRegistry tags;
  core::Actions actions;
  core::Random random{0};
  core::Timers timers;
  core::Vec2 gravity;
  std::unique_ptr<ecs::World> world;
  std::string save_path;
  // Assets, preloaded at boot; file paths resolve against `asset_dir`.
  std::filesystem::path asset_dir;
  std::map<std::string, core::SoundPtr, std::less<>> sounds;
  std::map<std::string, core::ImagePtr, std::less<>> images;
  core::Mixer mixer;
};

inline ScriptState& state_of(JSContext* ctx) { return *static_cast<ScriptState*>(JS_GetContextOpaque(ctx)); }

}  // namespace alpha::script
