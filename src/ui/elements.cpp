#include "ui/elements.hpp"

#include <RmlUi/Core/Element.h>

#include "core/error.hpp"

namespace alpha::ui {

void toggle_class(Rml::Element& element, const std::string& name, bool on) {
  if (name.empty() || name.find_first_of(" \t\n") != std::string::npos) {
    throw core::Error("class name must be one word, got \"" + name + "\"");
  }
  element.SetClass(name, on);
}

void set_attribute(Rml::Element& element, const std::string& name, const std::optional<std::string>& value) {
  if (name.empty() || name == "id") throw core::Error("attribute \"" + name + "\" cannot be set (elements are found by id)");
  if (name.rfind("on", 0) == 0) throw core::Error("attribute \"" + name + "\": inline handlers are not supported; use ctx.ui.on");
  if (value) element.SetAttribute(name, *value);
  else element.RemoveAttribute(name);
}

std::string classes_of(const Rml::Element& element) { return element.GetClassNames(); }

std::map<std::string, std::string> attributes_of(const Rml::Element& element) {
  std::map<std::string, std::string> out;
  for (const auto& [name, variant] : element.GetAttributes()) out[name] = variant.Get<std::string>();
  return out;
}

}  // namespace alpha::ui
