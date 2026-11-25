#include "script/bundle_map.hpp"

#include <fstream>
#include <sstream>

#include "core/error.hpp"
#include "script/js_value.hpp"

namespace alpha::script {

namespace {

std::string read_file(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  std::ostringstream text;
  if (file) text << file.rdbuf();
  return text.str();
}

}  // namespace

std::optional<core::SourceMap> load_bundle_map(JSContext* ctx, const std::string& path) {
  const auto json = core::inline_source_map(read_file(path));
  if (!json) return std::nullopt;
  const Value map(ctx, JS_ParseJSON(ctx, json->c_str(), json->size(), "source map"));
  try {
    if (JS_IsException(map.get())) throw core::Error(take_exception(ctx));
    const Value mappings = map.prop("mappings");
    const Value sources = map.prop("sources");
    return core::SourceMap(string_arg(ctx, mappings.get(), "source map mappings"),
                           strings_arg(ctx, sources.get(), "source map sources"));
  } catch (const core::Error&) {
    return std::nullopt;
  }
}

}  // namespace alpha::script
