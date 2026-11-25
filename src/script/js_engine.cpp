#include "script/js_engine.hpp"

#include <fstream>
#include <sstream>

namespace alpha::script {

Engine::Engine() : runtime_(JS_NewRuntime()), ctx_(JS_NewContext(runtime_)) {
  JS_SetModuleLoaderFunc(runtime_, nullptr, &Engine::load, this);
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
  Value compiled = check(compile(name));
  auto* module = static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(compiled.get()));
  const Value promise = check(JS_EvalFunction(ctx_, compiled.release()));
  run_jobs();
  if (JS_PromiseState(ctx_, promise.get()) == JS_PROMISE_REJECTED) {
    JS_Throw(ctx_, JS_PromiseResult(ctx_, promise.get()));
    throw ScriptError(take_exception(ctx_));
  }
  return {ctx_, JS_GetModuleNamespace(ctx_, module)};
}

Value Engine::check(JSValue result) {
  if (JS_IsException(result)) throw ScriptError(take_exception(ctx_));
  return {ctx_, result};
}

Value Engine::call(const Value& fn, std::span<JSValueConst> args) {
  Value result = check(JS_Call(ctx_, fn.get(), JS_UNDEFINED, static_cast<int>(args.size()),
                               args.data()));
  run_jobs();
  return result;
}

}  // namespace alpha::script
