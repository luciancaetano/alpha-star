#include "ui/library.hpp"

#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Log.h>
#include <RmlUi/Core/SystemInterface.h>
#include <SDL3/SDL.h>

#include <map>
#include <memory>
#include <vector>

#include "core/error.hpp"
#include "platform/vfs.hpp"
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

void load_font(const std::string& path, const std::string& family) {
  const std::string key = family + "|" + path;
  if (global->fonts.contains(key)) return;
  auto read = platform::vfs::read(path);
  if (!read) throw core::Error("cannot open font file " + path);
  std::vector<Rml::byte> bytes = std::move(*read);
  take_messages();
  if (!Rml::LoadFontFace({bytes.data(), bytes.size()}, family, Rml::Style::FontStyle::Normal,
                         Rml::Style::FontWeight::Auto, false)) {
    throw core::Error(path + " is not a font RmlUi can load (use .ttf or .otf): " + take_messages());
  }
  global->fonts[key] = std::move(bytes);  // the heap buffer RmlUi points at stays put
}

}  // namespace alpha::ui
