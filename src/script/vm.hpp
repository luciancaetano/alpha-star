#pragma once

#include <memory>
#include <optional>
#include <string>

#include "core/source_map.hpp"
#include "script/instances.hpp"
#include "script/js_engine.hpp"

namespace alpha::script {

using SavePathFor = std::function<std::string(const std::string& title)>;

// One complete script world: a QuickJS engine with the natives installed, its
// game instances (native state) and the bundle it imported. Host owns one;
// hot reload boots a second and swaps it in only when it booted cleanly.
// Never copied or moved: the JS context points at `instances`.
struct Vm {
  Vm(SavePathFor save_path_for, core::LogRing& log);
  Vm(const Vm&) = delete;
  Vm& operator=(const Vm&) = delete;

  // Imports the user's bundle (game or tests) and remembers its source map.
  Value import_bundle(const std::string& path);
  // An error message with bundle locations mapped to the original sources.
  std::string located(const std::exception& e) const;
  // The runtime's entry point `name` (boot, step, sceneName, ...). The
  // runtime module is imported once per Vm: Engine::import evaluates a new
  // module instance on every call, and the entry points must share one.
  Value runtime(const char* name);
  ScriptState& main() { return instances.main(); }

  // Declared in destruction order: values die before the engine, the engine
  // before the native state it points at.
  Instances instances;
  std::unique_ptr<Engine> engine;
  std::string bundle_path;
  std::optional<core::SourceMap> source_map;
  std::optional<Value> runtime_ns;
  std::optional<Value> step_fn;
  std::optional<Value> scene_fn;
};

}  // namespace alpha::script
