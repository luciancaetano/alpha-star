#include "script/prefs_bindings.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>

#include "core/error.hpp"
#include "platform/vfs.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"

namespace alpha::script {

namespace {

// <pref dir of the game>/<name>.json; name is letters, digits, '-' or '_'.
std::filesystem::path prefs_file(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::string title = string_arg(ctx, argv[0], "config.title");
  const std::string name = string_arg(ctx, argv[1], "prefs file");
  const bool plain = !name.empty() && name.size() <= 40 &&
                     std::all_of(name.begin(), name.end(), [](unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; });
  if (!plain) throw core::Error("prefs file \"" + name + "\": use 1..40 letters, digits, '-' or '_'");
  const std::string anchor = s.save_path_for(title);
  if (anchor.empty()) throw core::Error("this game instance has no save directory");
  return std::filesystem::path(anchor).parent_path() / (name + ".json");
}

// prefsRead(title, name, backup): the file's text (its .bak when backup), or null.
JSValue prefs_read(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  std::filesystem::path path = prefs_file(ctx, s, argv);
  if (JS_ToBool(ctx, argv[2]) == 1) path += ".bak";
  const auto text = platform::vfs::read_text(path);
  return text ? JS_NewStringLen(ctx, text->data(), text->size()) : JS_NULL;
}

// prefsWrite(title, name, text): atomic, keeping the previous file as .bak.
JSValue prefs_write(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  platform::vfs::write_atomic(prefs_file(ctx, s, argv), string_arg(ctx, argv[2], "save data"));
  return JS_UNDEFINED;
}

constexpr std::array<Native, 2> kNatives{{
    {"prefsRead", guarded<prefs_read>, 3},
    {"prefsWrite", guarded<prefs_write>, 3},
}};

}  // namespace

void install_prefs_bindings(JSContext* ctx, JSValueConst target) { install(ctx, target, kNatives); }

}  // namespace alpha::script
