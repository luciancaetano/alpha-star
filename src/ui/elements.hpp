#pragma once

#include <map>
#include <optional>
#include <string>

namespace Rml {
class Element;
}  // namespace Rml

namespace alpha::ui {

// ctx.ui.toggle: adds (`on`) or removes one class. Throws core::Error for an empty
// name or one with spaces (a class name is one word).
void toggle_class(Rml::Element& element, const std::string& name, bool on);
// ctx.ui.attr: sets an attribute (`value`) or removes it (nullopt), e.g. an <img>
// src, a <progress> value, disabled. Throws core::Error for "id" (scripts find
// elements by it) and "on..." names (inline handlers; use ctx.ui.on).
void set_attribute(Rml::Element& element, const std::string& name, const std::optional<std::string>& value);
// What tests read back: the element's classes ("a b") and attributes.
std::string classes_of(const Rml::Element& element);
std::map<std::string, std::string> attributes_of(const Rml::Element& element);

}  // namespace alpha::ui
