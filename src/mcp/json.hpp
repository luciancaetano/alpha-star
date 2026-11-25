#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace alpha::mcp::json {

// The control protocol's requests are flat JSON objects: every value is
// null, a boolean, a number or a string (no nesting), which is all a command
// needs and keeps the parser small.
using Field = std::variant<std::nullptr_t, bool, double, std::string>;
using Object = std::map<std::string, Field, std::less<>>;

// Parses one flat object. Throws core::Error saying what is wrong and where
// (offset), e.g. a nested value or a missing quote.
Object parse_object(std::string_view text);

// Writers: each returns JSON text; compose them into larger values.
std::string str(std::string_view s);  // quoted and escaped
std::string num(double v);            // finite numbers; NaN/inf become null
std::string boolean(bool v);
// {"k": raw, ...} from already-serialized values, in the order given.
std::string object(const std::vector<std::pair<std::string, std::string>>& fields);
std::string array(const std::vector<std::string>& items);
// A field value as JSON text (for echoing a request id).
std::string raw(const Field& f);

}  // namespace alpha::mcp::json
