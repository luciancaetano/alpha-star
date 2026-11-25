#pragma once

#include <quickjs.h>

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

// One QuickJS runtime + context. Modules come from built-in sources
// ("alpha", ...) or from files.
class Engine {
 public:
  Engine();
  ~Engine();
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  JSContext* ctx() const { return ctx_; }
  void add_builtin(std::string name, std::string_view source);
  // Evaluates a module (builtin name or file path); returns its namespace.
  Value import(const std::string& name);
  // Calls fn(args...) with `this` undefined. Throws ScriptError if it throws.
  Value call(const Value& fn, std::span<JSValueConst> args);
  // Wraps a result, turning a pending exception into ScriptError.
  Value check(JSValue result);

 private:
  static JSModuleDef* load(JSContext* ctx, const char* name, void* opaque);
  std::string source_of(const std::string& name) const;
  JSValue compile(const std::string& name);
  void run_jobs();

  JSRuntime* runtime_;
  JSContext* ctx_;
  std::map<std::string, std::string, std::less<>> builtins_;
};

}  // namespace alpha::script
