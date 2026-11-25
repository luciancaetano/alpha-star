#pragma once

#include <quickjs.h>

#include <exception>

#include "script/script_state.hpp"

namespace alpha::script {

using NativeImpl = JSValue (*)(JSContext*, ScriptState&, JSValueConst*);

// Adapts a native to QuickJS: any C++ exception becomes a JS TypeError with
// the same message, so a script can never crash the engine.
template <NativeImpl F>
JSValue guarded(JSContext* ctx, JSValueConst, int, JSValueConst* argv) {
  try {
    return F(ctx, state_of(ctx), argv);
  } catch (const std::exception& e) {
    return JS_ThrowTypeError(ctx, "%s", e.what());
  }
}

}  // namespace alpha::script
