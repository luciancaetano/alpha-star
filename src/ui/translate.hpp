#pragma once

#include <map>
#include <set>
#include <string>

namespace Rml {
class Element;
}

namespace alpha::ui {

// Elements with data-t="key" show the translation of `key` (plain text).
// Adds the keys used under `root` to `keys`.
void collect_translation_keys(Rml::Element* root, std::set<std::string>& keys);
// Sets the text of every data-t element under `root` that has a text in `texts`.
void apply_translations(Rml::Element* root, const std::map<std::string, std::string>& texts);

}  // namespace alpha::ui
