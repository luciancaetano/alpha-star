#include "script/js_value.hpp"

#include <utility>

#include "core/error.hpp"
#include "core/limits.hpp"
#include "core/utf8.hpp"

namespace alpha::script {

Value::~Value() { JS_FreeValue(ctx_, value_); }

Value::Value(Value&& other) noexcept
    : ctx_(other.ctx_), value_(std::exchange(other.value_, JS_UNDEFINED)) {}

Value& Value::operator=(Value&& other) noexcept {
  std::swap(ctx_, other.ctx_);
  std::swap(value_, other.value_);
  return *this;
}

JSValue Value::release() { return std::exchange(value_, JS_UNDEFINED); }

Value Value::prop(const char* name) const { return {ctx_, JS_GetPropertyStr(ctx_, value_, name)}; }

Value Value::at(std::uint32_t index) const {
  return {ctx_, JS_GetPropertyUint32(ctx_, value_, index)};
}

std::string to_string(JSContext* ctx, JSValueConst v) {
  const char* text = JS_ToCString(ctx, v);
  if (!text) {
    JS_FreeValue(ctx, JS_GetException(ctx));
    return "<unprintable>";
  }
  std::string out(text);
  JS_FreeCString(ctx, text);
  return out;
}

std::string describe(JSContext* ctx, JSValueConst v) {
  constexpr std::size_t kShown = 120;
  int64_t length = 0;
  if (JS_IsArray(v) && JS_GetLength(ctx, v, &length) == 0 && length > 64) {
    return "an array of " + std::to_string(length) + " items";  // String() of it may take seconds
  }
  const std::string text = to_string(ctx, v);
  return text.size() <= kShown ? text : text.substr(0, kShown) + "...";
}

std::string take_exception(JSContext* ctx) {
  const Value error(ctx, JS_GetException(ctx));
  std::string text = to_string(ctx, error.get());
  const Value stack = error.prop("stack");
  if (JS_IsString(stack.get())) text += "\n" + to_string(ctx, stack.get());
  return text;
}

double number_arg(JSContext* ctx, JSValueConst v, const std::string& what) {
  double out = 0.0;
  if (!JS_IsNumber(v) || JS_ToFloat64(ctx, &out, v) != 0) {
    throw core::Error(what + " must be a number, got " + describe(ctx, v));
  }
  return out;
}

std::string string_arg(JSContext* ctx, JSValueConst v, const std::string& what) {
  if (!JS_IsString(v)) throw core::Error(what + " must be a string, got " + describe(ctx, v));
  return to_string(ctx, v);
}

double dt_arg(JSContext* ctx, JSValueConst v) {
  const double dt = number_arg(ctx, v, "dt");
  if (!(dt >= 0.0 && dt <= 1.0)) throw core::Error("dt must be 0..1 seconds (one step), got " + std::to_string(dt));
  return dt;
}

std::string text_arg(JSContext* ctx, JSValueConst v, const std::string& what) {
  std::string text = to_string(ctx, v);
  if (text.size() > core::kMaxTextChars) {
    throw core::Error(what + " is " + std::to_string(text.size()) + " bytes long; displayed text is at most " +
                      std::to_string(core::kMaxTextChars));
  }
  if (const auto fault = core::utf8_fault(text)) {
    throw core::Error(what + " is not valid Unicode text: " + fault->why + " at UTF-8 byte " + std::to_string(fault->offset) +
                      " of \"" + text.substr(0, fault->offset) + "\"... (build strings from whole characters; slicing a JS string can split an emoji's surrogate pair)");
  }
  return text;
}

namespace {

// Every item of a JS array read by `read(ctx, item, what[i])`.
template <typename T, typename Read>
std::vector<T> array_of(JSContext* ctx, JSValueConst v, const std::string& what, const char* kind, Read read) {
  if (!JS_IsArray(v)) throw core::Error(what + " must be an array of " + kind + ", got " + describe(ctx, v));
  int64_t length = 0;
  JS_GetLength(ctx, v, &length);
  if (length > static_cast<int64_t>(core::kMaxArrayItems)) {
    throw core::Error(what + " has " + std::to_string(length) + " items; at most " +
                      std::to_string(core::kMaxArrayItems) + " are allowed");
  }
  std::vector<T> out;
  for (int64_t i = 0; i < length; ++i) {
    const Value item(ctx, JS_GetPropertyUint32(ctx, v, static_cast<std::uint32_t>(i)));
    out.push_back(read(ctx, item.get(), what + "[" + std::to_string(i) + "]"));
  }
  return out;
}

}  // namespace

std::vector<std::string> strings_arg(JSContext* ctx, JSValueConst v, const std::string& what) {
  return array_of<std::string>(ctx, v, what, "strings", string_arg);
}

std::vector<double> numbers_arg(JSContext* ctx, JSValueConst v, const std::string& what) {
  return array_of<double>(ctx, v, what, "numbers", number_arg);
}

core::Vec2 vec2_arg(JSContext* ctx, JSValueConst v, const std::string& what) {
  int64_t length = 0;
  if (!JS_IsArray(v) || JS_GetLength(ctx, v, &length) != 0 || length != 2) {
    throw core::Error(what + " must be [x, y], got " + describe(ctx, v));
  }
  const Value x(ctx, JS_GetPropertyUint32(ctx, v, 0));
  const Value y(ctx, JS_GetPropertyUint32(ctx, v, 1));
  return {static_cast<float>(number_arg(ctx, x.get(), what + "[0]")),
          static_cast<float>(number_arg(ctx, y.get(), what + "[1]"))};
}

JSValue new_vec2(JSContext* ctx, core::Vec2 v) {
  JSValue array = JS_NewArray(ctx);
  JS_SetPropertyUint32(ctx, array, 0, JS_NewFloat64(ctx, v.x));
  JS_SetPropertyUint32(ctx, array, 1, JS_NewFloat64(ctx, v.y));
  return array;
}

namespace {

template <typename T, typename Make>
JSValue new_array(JSContext* ctx, std::span<const T> items, Make make) {
  JSValue array = JS_NewArray(ctx);
  for (std::uint32_t i = 0; i < items.size(); ++i) JS_SetPropertyUint32(ctx, array, i, make(items[i]));
  return array;
}

}  // namespace

JSValue new_ids(JSContext* ctx, std::span<const std::uint32_t> ids) {
  return new_array(ctx, ids, [ctx](std::uint32_t id) { return JS_NewUint32(ctx, id); });
}

JSValue new_numbers(JSContext* ctx, std::span<const double> items) {
  return new_array(ctx, items, [ctx](double v) { return JS_NewFloat64(ctx, v); });
}

JSValue new_strings(JSContext* ctx, std::span<const std::string> items) {
  return new_array(ctx, items, [ctx](const std::string& s) { return JS_NewString(ctx, s.c_str()); });
}

void install(JSContext* ctx, JSValueConst target, std::span<const Native> natives) {
  for (const Native& n : natives) {
    JS_SetPropertyStr(ctx, target, n.name, JS_NewCFunction(ctx, n.fn, n.name, n.argc));
  }
}

}  // namespace alpha::script
