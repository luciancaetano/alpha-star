#pragma once

#include <quickjs.h>

#include <chrono>
#include <map>
#include <span>
#include <string>
#include <string_view>

#include "core/error.hpp"
#include "script/js_value.hpp"

namespace alpha::script {

// A JS exception escaped into the engine. what() = message + stack.
struct ScriptError : core::Error {
  using core::Error::Error;
};

// Limits on one script call (import, enter, update, handler): a script that
// exceeds either is stopped and the game pauses with a ScriptError.
inline constexpr std::chrono::milliseconds kCallBudget{250};
inline constexpr std::size_t kMemoryLimitMb = 256;

// One QuickJS runtime + context. Modules come from built-in sources
// ("alpha", ...) or from files.
class Engine {
 public:
  Engine();
  ~Engine();
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  // The engine that owns `ctx`.
  static Engine& of(JSContext* ctx);
  JSContext* ctx() const { return ctx_; }
  // A fresh call budget for the running script (simulate: one per game step).
  void restart_budget() { start_clock(); }
  // Wall time of the last call(), in milliseconds.
  double last_call_ms() const { return last_call_ms_; }
  void add_builtin(std::string name, std::string_view source);
  // Evaluates a module (builtin name or file path); returns its namespace.
  Value import(const std::string& name);
  // Calls fn(args...) with `this` undefined. Throws ScriptError if it throws.
  Value call(const Value& fn, std::span<JSValueConst> args);
  // Wraps a result, turning a pending exception into ScriptError.
  Value check(JSValue result);

 private:
  static JSModuleDef* load(JSContext* ctx, const char* name, void* opaque);
  static int interrupt(JSRuntime* rt, void* opaque);
  std::string limit_hit() const;
  void start_clock();
  void stop_clock();
  std::string source_of(const std::string& name) const;
  std::string public_builtins() const;
  JSValue compile(const std::string& name);
  void run_jobs();

  JSRuntime* runtime_;
  JSContext* ctx_;
  std::map<std::string, std::string, std::less<>> builtins_;
  std::chrono::steady_clock::time_point deadline_;
  bool timed_out_ = false;
  std::chrono::steady_clock::time_point started_;
  double last_call_ms_ = 0.0;
};

}  // namespace alpha::script
