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
  FontEngine engine;  // every font: platform font sources (world text shares them)
};

int users = 0;
std::unique_ptr<Global> global;

}  // namespace

Library::Library() {
  if (users++ > 0) return;
  global = std::make_unique<Global>();
  Rml::SetSystemInterface(&global->system);
  Rml::SetFileInterface(&global->files);
  Rml::SetFontEngineInterface(&global->engine);
  if (!Rml::Initialise()) throw core::Error("RmlUi failed to initialise: " + take_messages());
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

core::FontSourcePtr load_font(const std::string& path, const std::string& family, bool pixel_mode,
                              const std::vector<core::FontSourcePtr>& fallbacks) {
  core::FontSourcePtr font = platform::load_font(path, pixel_mode, fallbacks);
  global->engine.add_font(family, font);
  return font;
}

void set_density(float density) {
  if (global && global->engine.set_density(density)) Rml::ReleaseFontResources();
}

}  // namespace alpha::ui
