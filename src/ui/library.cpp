#include "ui/library.hpp"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/SystemInterface.h>
#include <SDL3/SDL.h>

#include <map>
#include <memory>
#include <vector>

#include "core/error.hpp"
#include "platform/fonts.hpp"
#include "platform/vfs.hpp"
#include "ui/font_engine.hpp"
#include "ui/pack_files.hpp"

namespace alpha::ui {

namespace {

// Wall-clock time for RmlUi animations; warnings and errors go to stderr and
// are kept until take_messages() so failures can name RmlUi's reason.
class System final : public Rml::SystemInterface {
 public:
  double GetElapsedTime() override { return static_cast<double>(SDL_GetTicksNS()) / 1e9; }
  bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
    if (type == Rml::Log::LT_ERROR || type == Rml::Log::LT_WARNING) {
      SDL_Log("[ui] %s", message.c_str());
      messages += (messages.empty() ? "" : "\n") + message;
    }
    return true;
  }
  std::string messages;
};

struct Global {
  System system;
  PackFiles files;
  // Font bytes must outlive RmlUi's use of them (until Shutdown).
  // Keyed by "family|path": loading the same font again (every simulate run
  // preloads its game's fonts) is a no-op.
  std::map<std::string, std::vector<Rml::byte>> fonts;
  std::unique_ptr<FontEngine> engine;  // wraps RmlUi's FreeType engine
};

int users = 0;
std::unique_ptr<Global> global;

}  // namespace

Library::Library() {
  if (users++ > 0) return;
  global = std::make_unique<Global>();
  Rml::SetSystemInterface(&global->system);
  Rml::SetFileInterface(&global->files);
  if (!Rml::Initialise()) throw core::Error("RmlUi failed to initialise: " + take_messages());
  global->engine = std::make_unique<FontEngine>(*Rml::GetFontEngineInterface());
  Rml::SetFontEngineInterface(global->engine.get());
}

Library::~Library() {
  if (--users > 0) return;
  Rml::Shutdown();
  global.reset();
}

std::string take_messages() {
  if (!global) return "";
  return std::exchange(global->system.messages, std::string());
}

namespace {

// TrueType / OpenType for RmlUi's FreeType engine (kept until RmlUi shuts down).
void register_truetype(const std::string& path, const std::string& family) {
  const std::string key = family + "|" + path;
  if (global->fonts.contains(key)) return;
  auto read = platform::vfs::read(path);
  if (!read) throw core::Error("cannot open font file " + path);
  std::vector<Rml::byte> bytes = std::move(*read);
  take_messages();
  global->engine->loaded_font();
  if (!global->engine->inner().LoadFontFace({bytes.data(), bytes.size()}, 0, family, Rml::Style::FontStyle::Normal,
                                            Rml::Style::FontWeight::Auto, false)) {
    throw core::Error(path + " is not a font RmlUi can load (use .ttf, .otf or a BMFont .fnt): " + take_messages());
  }
  global->fonts[key] = std::move(bytes);  // the heap buffer RmlUi points at stays put
}

}  // namespace


core::FontSourcePtr load_font(const std::string& path, const std::string& family, bool pixel_mode) {
  if (platform::is_bitmap_font(path)) {
    core::FontSourcePtr font = platform::load_font(path, pixel_mode);
    global->engine->add_bitmap(family, font);
    return font;
  }
  register_truetype(path, family);
  return platform::load_font(path, pixel_mode);
}

void set_density(float density) {
  if (global && global->engine->set_density(density)) Rml::ReleaseFontResources();
}

}  // namespace alpha::ui
