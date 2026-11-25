#pragma once

#include <quickjs.h>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "core/vec2.hpp"

namespace alpha::script {

// Owning JSValue: frees on destruction. Move-only.
class Value {
 public:
  Value(JSContext* ctx, JSValue value) : ctx_(ctx), value_(value) {}
  ~Value();
  Value(Value&& other) noexcept;
  Value& operator=(Value&& other) noexcept;
  Value(const Value&) = delete;
  Value& operator=(const Value&) = delete;

  JSValueConst get() const { return value_; }
  // Gives up ownership (for APIs that consume the value).
  JSValue release();
  Value prop(const char* name) const;
  Value at(std::uint32_t index) const;

 private:
  JSContext* ctx_;
  JSValue value_;
};

// Text of any JS value (String(v)).
std::string to_string(JSContext* ctx, JSValueConst v);
// Message and stack of the pending exception; clears it.
std::string take_exception(JSContext* ctx);

// Argument readers: throw core::Error naming `what` when the type is wrong.
double number_arg(JSContext* ctx, JSValueConst v, const std::string& what);
std::string string_arg(JSContext* ctx, JSValueConst v, const std::string& what);
std::vector<std::string> strings_arg(JSContext* ctx, JSValueConst v, const std::string& what);
core::Vec2 vec2_arg(JSContext* ctx, JSValueConst v, const std::string& what);
std::vector<double> numbers_arg(JSContext* ctx, JSValueConst v, const std::string& what);

JSValue new_vec2(JSContext* ctx, core::Vec2 v);
JSValue new_ids(JSContext* ctx, std::span<const std::uint32_t> ids);
JSValue new_strings(JSContext* ctx, std::span<const std::string> items);
JSValue new_numbers(JSContext* ctx, std::span<const double> items);

// A native function exposed on an object.
struct Native {
  const char* name;
  JSCFunction* fn;
  int argc;
};
void install(JSContext* ctx, JSValueConst target, std::span<const Native> natives);

}  // namespace alpha::script
