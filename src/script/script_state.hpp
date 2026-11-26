#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "core/image.hpp"
#include "core/controls.hpp"
#include "core/glyph_layout.hpp"
#include "core/window.hpp"
#include "core/mixer.hpp"
#include "core/perf.hpp"
#include "core/random.hpp"
#include "core/shader.hpp"
#include "core/sheet.hpp"
#include "core/tags.hpp"
#include "core/timers.hpp"
#include "ecs/world.hpp"
#include "net/hub.hpp"
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
  core::Vec2 design_size;  // config.size: each scene's camera starts centered on it
  std::optional<core::Vec2> screen;  // the real screen in output pixels (engine; Host::set_screen)
  core::Vec2 view_size;    // what the screen shows: config.size grown by config.window.aspect
  std::unique_ptr<ecs::World> world;
  // The game's save.json path for its title; save slots and settings sit next
  // to it. Empty for test instances (their saves live in memory).
  std::function<std::string(const std::string& title)> save_path_for;
  // Assets, preloaded at boot; file paths resolve against `asset_dir`.
  std::filesystem::path asset_dir;
  struct Sound {
    core::SoundPtr samples;
    core::BusId bus = core::Mixer::kSfx;
  };
  std::map<std::string, Sound, std::less<>> sounds;
  // assets.music: full paths, streamed when played (never preloaded).
  std::map<std::string, std::string, std::less<>> music;
  core::PictureTable images;
  core::SheetTable sheets;
  // Decoded image files by full path: an atlas page or tileset is decoded once.
  std::map<std::string, core::ImagePtr, std::less<>> decoded;
  // assets.fonts by name (the game UI loads them too) and the atlases that
  // size world-text boxes, per (font, pixel size).
  std::map<std::string, core::FontSourcePtr, std::less<>> fonts;
  // assets.shaders by name, with their compiled blobs (custom postfx effects).
  std::map<std::string, core::ShaderPtr, std::less<>> shaders;
  // config.collision.categories, in order (category i has bit i + 1).
  std::vector<std::string> categories;
  // config.pixelMode false: filled shapes get antialiased edges unless shape.smooth says otherwise.
  bool smooth_shapes = false;
  // config.lighting (none: no lightmap; lights are ignored).
  std::optional<core::Lighting> lighting;
  // config.layers by name.
  std::map<std::string, core::Layer, std::less<>> layers;
  // ctx.window: written by the engine every frame (headless: as scripts set it).
  core::WindowState window;
  core::Mixer mixer;
  // ctx.locale (mirrored for MCP state and dev tools; the runtime owns it).
  std::string locale;
  // ctx.settings.reducedMotion: camera shake and particle counts scaled by
  // core::kReducedMotion.
  bool reduced_motion = false;
  // Game UI (assets.ui documents), created at boot with config.size.
  std::unique_ptr<ui::Ui> ui;
  // Time natives spent this frame by section (timers, physics, ...); Host collects it.
  core::FrameTimes times;
  // alpha:net sockets, made on the first one; destroyed first, closing them all.
  std::unique_ptr<net::Hub> net;
};


}  // namespace alpha::script
