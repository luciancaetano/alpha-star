#pragma once

// Shared helpers for Host tests: a temp directory per test and a tiny game.
#include <filesystem>
#include <string>

#include "core/input.hpp"
#include "script/host.hpp"

namespace alpha::test {

struct Sandbox {
  std::filesystem::path dir = std::filesystem::temp_directory_path() / "alpha_host_test";
  Sandbox();
  ~Sandbox();
  Sandbox(const Sandbox&) = delete;
  Sandbox& operator=(const Sandbox&) = delete;
  // Writes `source` to dir/name; returns the full path.
  std::string write(const std::string& name, const std::string& source) const;
  // Save location for a Host: always dir/save.json, whatever the title.
  script::SavePathFor save() const;
  std::string save_file() const;
};

core::InputSet space();
std::string first_text(script::Host& host);
// A game whose `play` scene runs `body` in update (or enter).
std::string tiny(const std::string& hook, const std::string& body);

}  // namespace alpha::test
