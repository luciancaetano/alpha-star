#pragma once

// Shared helpers for Host tests: a temp directory per test and a tiny game.
#include <filesystem>
#include <string>
#include <vector>

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

core::InputFrame space();
// Text of element `id` in whichever shown UI document has it; "" if none.
std::string ui_text(script::Host& host, const std::string& id);
// RML document with one element per id: <p id="...">.
std::string rml(const std::vector<std::string>& ids);
// A game whose `play` scene runs `body` in update (or enter).
std::string tiny(const std::string& hook, const std::string& body);
// A 400 x 300 game whose `play` scene is `{ <scene> }` (enter, update, state), with
// `config` appended to its config, `vec` imported, check(ok, what) and near(a, b, eps?).
std::string game_with(const std::string& config, const std::string& scene);

}  // namespace alpha::test
