#include "platform/log_file.hpp"

#include <SDL3/SDL.h>

#include <cstdio>

#include "platform/vfs.hpp"

namespace alpha::platform {

namespace {

// SDL_Log lines join console.* in the same stream (stderr, i.e. the log file).
void to_stderr(void*, int, SDL_LogPriority, const char* message) {
  std::fprintf(stderr, "%s\n", message);
  std::fflush(stderr);
}

}  // namespace

std::string pref_file(const std::string& title, const char* name) {
  char* dir = SDL_GetPrefPath("alpha-star", title.c_str());
  if (!dir) {
    SDL_Log("no per-user data dir (%s); %s disabled", SDL_GetError(), name);
    return "";
  }
  std::string path = std::string(dir) + name;
  SDL_free(dir);
  return path;
}

std::string text_of(const core::LogRing& ring) {
  std::string text;
  for (std::size_t i = 0; i < ring.size(); ++i) text += core::format_line(ring.at(i));
  return text;
}

bool redirect_logs(const std::string& path, const core::LogRing& earlier) {
  if (!vfs::redirect_stderr(path)) return false;  // also for "" (no pref dir)
  std::setvbuf(stderr, nullptr, _IONBF, 0);  // a killed game still leaves its log
  std::fputs(text_of(earlier).c_str(), stderr);
  std::fflush(stderr);
  SDL_SetLogOutputFunction(to_stderr, nullptr);
  return true;
}

}  // namespace alpha::platform
