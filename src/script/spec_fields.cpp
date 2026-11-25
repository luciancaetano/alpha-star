#include "script/spec_fields.hpp"

namespace alpha::script {

bool has(JSContext* ctx, JSValueConst obj, const char* name) {
  const Value v(ctx, JS_GetPropertyStr(ctx, obj, name));
  return !JS_IsUndefined(v.get());
}

Value field(JSContext* ctx, JSValueConst obj, const char* name) { return {ctx, JS_GetPropertyStr(ctx, obj, name)}; }

core::Color color_arg(JSContext* ctx, JSValueConst v, const std::string& path) {
  const std::string text = string_arg(ctx, v, kSpawn + path);
  const auto parsed = core::parse_color(text);
  if (!parsed) throw core::Error(kSpawn + path + " must be a color like \"#73bf2e\" or \"#ffffff80\", got \"" + text + "\"");
  return *parsed;
}

float number_or(JSContext* ctx, JSValueConst obj, const char* name, const std::string& path, float fallback) {
  if (!has(ctx, obj, name)) return fallback;
  return static_cast<float>(number_arg(ctx, field(ctx, obj, name).get(), kSpawn + path));
}

}  // namespace alpha::script
