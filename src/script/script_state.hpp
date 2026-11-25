#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "core/image.hpp"
#include "core/controls.hpp"
#include "core/mixer.hpp"
#include "core/random.hpp"
#include "core/sheet.hpp"
#include "core/tags.hpp"
#include "core/timers.hpp"
#include "ecs/world.hpp"
#include "ui/ui.hpp"

namespace alpha::script {

// Engine state the natives act on: one per game instance (see Instances).
// The runtime replaces `world` on every scene enter.
struct ScriptState {
  core::TagRegistry tags;
  core::Controls controls;
  core::Random random{0};
  core::Timers timers;
  core::Vec2 gravity;
  core::Vec2 view_size;  // config.size: what each scene's camera shows
  std::unique_ptr<ecs::World> world;
  // ctx.save file: resolved from the game title when the runtime first reads it.
  std::function<std::string(const std::string& title)> save_path_for;
  std::string save_path;
  // Assets, preloaded at boot; file paths resolve against `asset_dir`.
  std::filesystem::path asset_dir;
  std::map<std::string, core::SoundPtr, std::less<>> sounds;
  core::PictureTable images;
  core::SheetTable sheets;
  // Decoded image files by full path: an atlas page or tileset is decoded once.
  std::map<std::string, core::ImagePtr, std::less<>> decoded;
  core::Mixer mixer;
  // Game UI (assets.ui documents), created at boot with config.size.
  std::unique_ptr<ui::Ui> ui;
};


}  // namespace alpha::script
