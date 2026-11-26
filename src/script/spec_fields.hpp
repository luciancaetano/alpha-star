#pragma once

#include <quickjs.h>

#include <array>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "core/color.hpp"
#include "core/error.hpp"
#include "core/index_of.hpp"
#include "script/js_value.hpp"

// Field access shared by the entity spec parsers. Errors start with "spawn: ".
namespace alpha::script {

inline const std::string kSpawn = "spawn: ";

bool has(JSContext* ctx, JSValueConst obj, const char* name);
Value field(JSContext* ctx, JSValueConst obj, const char* name);
// The object's own enumerable string keys, in insertion order.
std::vector<std::string> keys(JSContext* ctx, JSValueConst obj);
// A "#rgb" / "#rrggbb" / "#rrggbbaa" string; `path` names the field in errors.
core::Color color_arg(JSContext* ctx, JSValueConst v, const std::string& path);
// The same color check where `what` is the whole field name (no "spawn: " prefix).
core::Color color_named(JSContext* ctx, JSValueConst v, const std::string& what);
// obj[name] as a number, or `fallback` when absent.
float number_or(JSContext* ctx, JSValueConst obj, const char* name, const std::string& path, float fallback);
// A number in [lo, hi]; errors name `what`, the range and the value.
float ranged(JSContext* ctx, JSValueConst v, const std::string& what, double lo, double hi);

// One of `options` by name, or an error listing them ("a", "b" or "c").
template <typename E, std::size_t N>
E choice(JSContext* ctx, JSValueConst v, const std::string& path, const std::array<std::pair<const char*, E>, N>& options) {
  const std::string name = string_arg(ctx, v, kSpawn + path);
  std::string allowed;
  for (std::size_t i = 0; i < N; ++i) {
    if (name == options[i].first) return options[i].second;
    allowed += std::string(i == 0 ? "" : (i + 1 == N ? " or " : ", ")) + "\"" + options[i].first + "\"";
  }
  throw core::Error(kSpawn + path + " must be " + allowed + ", got \"" + name + "\"");
}

// assets.<kind>[name], or an error listing what the game declared.
template <typename Table>
const typename Table::mapped_type& named(const Table& table, const std::string& name, const std::string& field,
                                         const std::string& kind) {
  const auto it = table.find(name);
  if (it != table.end()) return it->second;
  const std::string known = core::keys_of(table, " ");
  throw core::Error(kSpawn + field + " \"" + name + "\" is not in " + kind + "; have: " +
                    (known.empty() ? "(none)" : known));
}

}  // namespace alpha::script
