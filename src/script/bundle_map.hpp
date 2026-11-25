#pragma once

#include <quickjs.h>

#include <optional>
#include <string>

#include "core/source_map.hpp"

namespace alpha::script {

// The inline source map esbuild appends to a bundled module file (`alpha
// export` / `alpha test`), or nullopt if the file has none or it is malformed:
// error locations then stay as bundle positions.
std::optional<core::SourceMap> load_bundle_map(JSContext* ctx, const std::string& path);

}  // namespace alpha::script
