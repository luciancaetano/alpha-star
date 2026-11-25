#include "script/bundle_map.hpp"

#include "core/error.hpp"
#include "platform/vfs.hpp"
#include "script/js_value.hpp"

namespace alpha::script {

std::optional<core::SourceMap> load_bundle_map(JSContext* ctx, const std::string& path) {
  const auto json = core::inline_source_map(platform::vfs::read_text(path).value_or(""));
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
