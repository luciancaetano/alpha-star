#include "ui/ui.hpp"

#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/ElementText.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/StringUtilities.h>

#include "core/error.hpp"

namespace alpha::ui {

namespace {

int contexts_made = 0;

template <typename F>
void each_element(Rml::Element* element, const F& visit) {
  visit(element);
  for (int i = 0; i < element->GetNumChildren(); ++i) each_element(element->GetChild(i), visit);
}

// Plain text inside an element (what a player reads), markup excluded.
std::string text_of(Rml::Element* element) {
  std::string out;
  each_element(element, [&](Rml::Element* e) {
    if (const auto* text = rmlui_dynamic_cast<Rml::ElementText*>(e)) out += text->GetText();
  });
  return out;
}

std::string ids_of(Rml::ElementDocument* doc) {
  std::string out;
  each_element(doc, [&](Rml::Element* e) {
    if (!e->GetId().empty()) out += (out.empty() ? "" : ", ") + e->GetId();
  });
  return out.empty() ? "(none)" : out;
}

bool form_control(const Rml::Element* e) {
  const std::string& tag = e->GetTagName();
  return tag == "input" || tag == "textarea" || tag == "select";
}

}  // namespace

void Ui::Listener::ProcessEvent(Rml::Event& event) { ui_.clicked(event.GetTargetElement()); }

Ui::Ui(int width, int height) : context_name_("game-ui-" + std::to_string(++contexts_made)) {
  context_ = Rml::CreateContext(context_name_, {width, height}, &renderer_);
  if (!context_) throw core::Error("cannot create the game UI: " + take_messages());
}

Ui::~Ui() {
  Rml::RemoveContext(context_name_);
  Rml::ReleaseRenderManagers();  // our renderer's textures and meshes, while it still exists
}

Rml::ElementDocument* Ui::load(const std::string& path) {
  take_messages();
  Rml::ElementDocument* doc = context_->LoadDocument(path);
  if (!doc) throw core::Error("cannot load " + path + ": " + take_messages());
  doc->AddEventListener(Rml::EventId::Click, &listener_);
  return doc;
}

void Ui::add(const std::string& name, const std::string& path) {
  if (const auto it = documents_.find(name); it != documents_.end()) context_->UnloadDocument(it->second.doc);
  documents_[name] = {path, load(path)};
}

Ui::Document& Ui::document(const std::string& name) {
  const auto it = documents_.find(name);
  if (it != documents_.end()) return it->second;
  std::string have;
  for (const auto& [n, d] : documents_) have += (have.empty() ? "" : ", ") + n;
  throw core::Error("unknown document \"" + name + "\"; assets.ui has: " + (have.empty() ? "(none)" : have));
}

Rml::Element& Ui::element(const std::string& name, const std::string& id) {
  Document& d = document(name);
  if (Rml::Element* e = d.doc->GetElementById(id)) return *e;
  throw core::Error("no element with id \"" + id + "\" in document \"" + name + "\" (" + d.path +
                    "); ids: " + ids_of(d.doc));
}

void Ui::show(const std::string& name) {
  Rml::ElementDocument* doc = document(name).doc;
  doc->Show();
  if (gamepad_last_) focus_first(doc);
}

void Ui::focus_first(Rml::ElementDocument* doc) {
  const Rml::Element* focus = context_->GetFocusElement();
  if (focus && focus != doc && focus->GetOwnerDocument() == doc) return;  // autofocus did it
  context_->Update();
  Rml::Element* first = nullptr;
  each_element(doc, [&](Rml::Element* e) {
    if (!first && e != doc && e->IsVisible() && e->GetComputedValues().tab_index() == Rml::Style::TabIndex::Auto) {
      first = e;
    }
  });
  if (first) first->Focus(true);
}

void Ui::hide(const std::string& name) { document(name).doc->Hide(); }

void Ui::set(const std::string& name, const std::string& id, const std::string& text) {
  element(name, id).SetInnerRML(Rml::StringUtilities::EncodeRml(text));
}

std::uint32_t Ui::on(const std::string& name, const std::string& id, const std::string& event) {
  if (event != "click" && event != "back") throw core::Error("unknown event \"" + event + "\"; events: click, back");
  element(name, id);  // must exist now
  const std::uint32_t handler = next_handler_++;
  handlers_[{name, id, event}] = handler;
  return handler;
}

void Ui::click(const std::string& name, const std::string& id) {
  Rml::Element& e = element(name, id);
  if (!document(name).doc->IsVisible()) throw core::Error("document \"" + name + "\" is hidden");
  e.Click();
}

std::vector<std::uint32_t> Ui::take_events() { return std::exchange(events_, {}); }

void Ui::reset() {
  for (auto& [name, d] : documents_) {
    context_->UnloadDocument(d.doc);
    d.doc = load(d.path);
  }
  handlers_.clear();
  events_.clear();
}

std::map<std::string, DocumentView> Ui::view() const {
  std::map<std::string, DocumentView> out;
  for (const auto& [name, d] : documents_) {
    DocumentView& v = out[name];
    v.shown = d.doc->IsVisible();
    each_element(d.doc, [&](Rml::Element* e) {
      if (!e->GetId().empty()) v.texts[e->GetId()] = text_of(e);
    });
  }
  return out;
}

std::string Ui::name_of(const Rml::ElementDocument* doc) const {
  for (const auto& [name, d] : documents_) {
    if (d.doc == doc) return name;
  }
  return "";
}

std::uint32_t Ui::handler_for(Rml::Element* element) const {
  const std::string name = element ? name_of(element->GetOwnerDocument()) : "";
  for (Rml::Element* e = element; e && !name.empty(); e = e->GetParentNode()) {
    const auto it = handlers_.find({name, e->GetId(), "click"});
    if (!e->GetId().empty() && it != handlers_.end()) return it->second;
  }
  return 0;
}

void Ui::clicked(Rml::Element* target) {
  if (const std::uint32_t handler = handler_for(target)) events_.push_back(handler);
}

bool Ui::back() {
  bool any = false;
  for (const auto& [key, handler] : handlers_) {
    const auto& [name, id, event] = key;
    if (event != "back" || !documents_.at(name).doc->IsVisible()) continue;
    events_.push_back(handler);
    any = true;
  }
  return any;
}

bool Ui::navigate(Nav nav) {
  if (nav == Nav::None) return false;
  const int shift = nav == Nav::Prev ? Rml::Input::KM_SHIFT : 0;
  context_->Update();
  const bool moved = !context_->ProcessKeyDown(Rml::Input::KI_TAB, shift);
  context_->ProcessKeyUp(Rml::Input::KI_TAB, shift);
  return moved;
}

bool Ui::interactive(Rml::Element* element) const {
  if (handler_for(element)) return true;
  for (Rml::Element* e = element; e; e = e->GetParentNode()) {
    if (form_control(e)) return true;
  }
  return false;
}

void Ui::attach(SDL_Renderer* sdl) {
  if (renderer_.attached() == sdl) return;
  Rml::ReleaseTextures(&renderer_);
  Rml::ReleaseCompiledGeometry(&renderer_);
  renderer_.attach(sdl);
}

void Ui::render(SDL_Renderer* sdl) {
  attach(sdl);
  context_->Update();
  renderer_.begin();
  context_->Render();
  renderer_.end();
}

}  // namespace alpha::ui
