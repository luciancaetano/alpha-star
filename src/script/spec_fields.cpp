#include "script/spec_fields.hpp"

#include <sstream>

namespace alpha::script {

bool has(JSContext* ctx, JSValueConst obj, const char* name) {
  const Value v(ctx, JS_GetPropertyStr(ctx, obj, name));
  return !JS_IsUndefined(v.get());
}

Value field(JSContext* ctx, JSValueConst obj, const char* name) { return {ctx, JS_GetPropertyStr(ctx, obj, name)}; }

core::Color color_named(JSContext* ctx, JSValueConst v, const std::string& what) {
  const std::string text = string_arg(ctx, v, what);
  const auto parsed = core::parse_color(text);
  if (!parsed) throw core::Error(what + " must be a color like \"#73bf2e\" or \"#ffffff80\", got \"" + text + "\"");
  return *parsed;
}

core::Color color_arg(JSContext* ctx, JSValueConst v, const std::string& path) { return color_named(ctx, v, kSpawn + path); }

float ranged(JSContext* ctx, JSValueConst v, const std::string& what, double lo, double hi) {
  const double x = number_arg(ctx, v, what);
  if (!(x >= lo && x <= hi)) {
    std::ostringstream out;
    out << what << " must be in [" << lo << ", " << hi << "], got " << x;
    throw core::Error(out.str());
  }
  return static_cast<float>(x);
}

float number_or(JSContext* ctx, JSValueConst obj, const char* name, const std::string& path, float fallback) {
  if (!has(ctx, obj, name)) return fallback;
  return static_cast<float>(number_arg(ctx, field(ctx, obj, name).get(), kSpawn + path));
}

std::vector<std::string> keys(JSContext* ctx, JSValueConst obj) {
  JSPropertyEnum* tab = nullptr;
  uint32_t len = 0;
  std::vector<std::string> out;
  if (JS_GetOwnPropertyNames(ctx, &tab, &len, obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) return out;
  for (uint32_t i = 0; i < len; ++i) {
    const char* name = JS_AtomToCString(ctx, tab[i].atom);
    if (name) out.emplace_back(name);
    JS_FreeCString(ctx, name);
  }
  JS_FreePropertyEnum(ctx, tab, len);
  return out;
}

}  // namespace alpha::script
