#include "ui/translate.hpp"

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/StringUtilities.h>

#include <utility>
#include <vector>

namespace alpha::ui {

namespace {

constexpr const char* kAttribute = "data-t";

using Keyed = std::vector<std::pair<Rml::Element*, std::string>>;

// Every element under `element` (itself included) with a data-t key.
void keyed(Rml::Element* element, Keyed& out) {
  const std::string key = element->GetAttribute<Rml::String>(kAttribute, "");
  if (!key.empty()) out.emplace_back(element, key);
  for (int i = 0; i < element->GetNumChildren(); ++i) keyed(element->GetChild(i), out);
}

Keyed keyed(Rml::Element* root) {
  Keyed out;
  keyed(root, out);
  return out;
}

}  // namespace

void collect_translation_keys(Rml::Element* root, std::set<std::string>& keys) {
  for (const auto& [element, key] : keyed(root)) keys.insert(key);
}

void apply_translations(Rml::Element* root, const std::map<std::string, std::string>& texts) {
  for (const auto& [element, key] : keyed(root)) {
    if (const auto it = texts.find(key); it != texts.end()) element->SetInnerRML(Rml::StringUtilities::EncodeRml(it->second));
  }
}

}  // namespace alpha::ui
