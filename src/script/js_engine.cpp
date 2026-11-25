#include "script/js_engine.hpp"

#include <fstream>
#include <sstream>

namespace alpha::script {

Engine::Engine() : runtime_(JS_NewRuntime()), ctx_(JS_NewContext(runtime_)) {
  JS_SetModuleLoaderFunc(runtime_, nullptr, &Engine::load, this);
  JS_SetMemoryLimit(runtime_, kMemoryLimitMb * 1024 * 1024);
  JS_SetInterruptHandler(runtime_, &Engine::interrupt, this);
  JS_SetRuntimeOpaque(runtime_, this);
}

Engine& Engine::of(JSContext* ctx) { return *static_cast<Engine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx))); }

int Engine::interrupt(JSRuntime*, void* opaque) {
  auto* self = static_cast<Engine*>(opaque);
  self->timed_out_ = std::chrono::steady_clock::now() > self->deadline_;
  return self->timed_out_ ? 1 : 0;
}

// Explains an exception caused by an engine limit, or returns "".
std::string Engine::limit_hit() const {
  if (timed_out_) {
    return "script ran longer than " + std::to_string(kCallBudget.count()) +
           " ms in one call and was stopped (infinite loop?)\n";
  }
  return "";
}

Engine::~Engine() {
  JS_FreeContext(ctx_);
  JS_FreeRuntime(runtime_);
}

void Engine::add_builtin(std::string name, std::string_view source) {
  builtins_[std::move(name)] = std::string(source);
}

std::string Engine::source_of(const std::string& name) const {
  if (const auto it = builtins_.find(name); it != builtins_.end()) return it->second;
  std::ifstream file(name, std::ios::binary);
  if (!file) throw core::Error("cannot open module \"" + name + "\"");
  std::ostringstream text;
  text << file.rdbuf();
  return text.str();
}

JSValue Engine::compile(const std::string& name) {
  const std::string source = source_of(name);
  return JS_Eval(ctx_, source.c_str(), source.size(), name.c_str(),
                 JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);
}

JSModuleDef* Engine::load(JSContext* ctx, const char* name, void* opaque) {
  auto* self = static_cast<Engine*>(opaque);
  try {
    const JSValue compiled = self->compile(name);
    if (JS_IsException(compiled)) return nullptr;
    auto* module = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(compiled));
    JS_FreeValue(ctx, compiled);
    return module;
  } catch (const core::Error& e) {
    JS_ThrowReferenceError(ctx, "%s", e.what());
    return nullptr;
  }
}

void Engine::run_jobs() {
  JSContext* job_ctx = nullptr;
  while (JS_ExecutePendingJob(runtime_, &job_ctx) > 0) {
  }
}

Value Engine::import(const std::string& name) {
  start_clock();
  Value compiled = check(compile(name));
  auto* module = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(compiled.get()));
  const Value promise = check(JS_EvalFunction(ctx_, compiled.release()));
  run_jobs();
  if (JS_PromiseState(ctx_, promise.get()) == JS_PROMISE_REJECTED) {
    check(JS_Throw(ctx_, JS_PromiseResult(ctx_, promise.get())));
  }
  return {ctx_, JS_GetModuleNamespace(ctx_, module)};
}

Value Engine::check(JSValue result) {
  if (!JS_IsException(result)) return {ctx_, result};
  std::string error = take_exception(ctx_);
  if (error.find("out of memory") != std::string::npos) {
    error = "script ran out of memory (limit " + std::to_string(kMemoryLimitMb) + " MB)\n" + error;
  }
  throw ScriptError(limit_hit() + error);
}

void Engine::start_clock() {
  deadline_ = std::chrono::steady_clock::now() + kCallBudget;
  timed_out_ = false;
}

Value Engine::call(const Value& fn, std::span<JSValueConst> args) {
  start_clock();
  Value result = check(JS_Call(ctx_, fn.get(), JS_UNDEFINED, static_cast<int>(args.size()),
                               args.data()));
  run_jobs();
  return result;
}

}  // namespace alpha::script
