#pragma once

#include <RmlUi/Core/EventListener.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "core/window.hpp"
#include "ui/library.hpp"
#include "ui/pad_nav.hpp"
#include "ui/rml_renderer.hpp"

namespace Rml {
class Context;
class Element;
class ElementDocument;
}  // namespace Rml

namespace alpha::ui {

// What tests may inspect of one document: visible or not, and the text of
// every element that has an id.
struct DocumentView {
  bool shown = false;
  std::map<std::string, std::string> texts;
  std::map<std::string, std::string> classes;                            // id -> "a b"
  std::map<std::string, std::map<std::string, std::string>> attributes;  // id -> name -> value
};

// One game's UI: an RmlUi context the size of the game's logical screen,
// holding the game's documents (assets.ui), loaded hidden at boot.
// Scene-scoped like everything else: reset() reloads every document hidden
// and drops all handlers. Clicks never call scripts directly: they queue the
// handler id, and the runtime runs queued handlers at the start of the next
// step (deterministic, and never re-entrant into a running script).
class Ui {
 public:
  // images: how UI images (img, decorators) are sampled when scaled (config.uiFilter).
  Ui(int width, int height, core::Filter images = core::Filter::linear);
  ~Ui();
  Ui(const Ui&) = delete;
  Ui& operator=(const Ui&) = delete;

  // Loads assets.ui.<name> from `path`. Throws core::Error with RmlUi's reason.
  void add(const std::string& name, const std::string& path);
  // With a gamepad as the last input device, focus moves to the document's
  // autofocus element, else its first focusable one (tab-index: auto).
  void show(const std::string& name);
  void hide(const std::string& name);
  // Replaces the element's content with `text` (plain text, never markup).
  void set(const std::string& name, const std::string& id, const std::string& text);
  // ui/elements.hpp rules: one class on/off; an attribute set or removed (nullopt).
  void toggle(const std::string& name, const std::string& id, const std::string& cls, bool on);
  void attr(const std::string& name, const std::string& id, const std::string& attr, const std::optional<std::string>& value);
  // Events: "click" (the element or anything inside it is clicked, by mouse,
  // touch, Enter/Space or gamepad A on focus) and "back" (Escape or gamepad B
  // while the element's document is shown). Each queues the returned handler
  // id. A second on() for the same element and event replaces the first.
  std::uint32_t on(const std::string& name, const std::string& id, const std::string& event);
  // A click as if the user did it (simulate). Throws if the document is hidden.
  void click(const std::string& name, const std::string& id);
  std::vector<std::uint32_t> take_events();
  void reset();
  std::map<std::string, DocumentView> view() const;
  // Safe-area insets (logical pixels, ctx.safeArea): every element with class
  // "safe" gets them as padding, so HUDs sit inside cutouts and rounded corners.
  void safe_area(const core::Insets& insets);
  // The logical screen changed size (config.window.aspect reveals more world).
  void resize(int width, int height);
  // data-t="key" elements: every key the documents use, and their texts (kept,
  // so documents reloaded later show them too).
  std::vector<std::string> translation_keys() const;
  void translate(std::map<std::string, std::string> texts);
  // Accessibility: `scale` = RmlUi dp ratio (sizes in dp grow with it);
  // high contrast = class "high-contrast" on every document's body.
  void accessibility(float scale, bool high_contrast);
  float scale() const { return scale_; }
  bool high_contrast() const { return high_contrast_; }

  // Engine side: drawing and input (see ui/input.hpp).
  void render(SDL_Renderer* sdl);
  // Switches the target renderer; textures of the old one are released first.
  void attach(SDL_Renderer* sdl);
  Rml::Context& context() { return *context_; }
  // True if `element` or an ancestor has a handler or is a form control.
  bool interactive(Rml::Element* element) const;
  // Called by the click listener with the event's target element.
  void clicked(Rml::Element* target);
  // Back (Escape / B): queues every "back" handler of shown documents; true if any.
  bool back();
  // Moves focus to the next / previous focusable element (Tab / Shift+Tab).
  bool navigate(Nav nav);
  // Repeat of a held D-pad / stick direction; call every frame (seconds).
  void tick(double now) { navigate(pad_.tick(now)); }
  PadNav& pad() { return pad_; }
  // Which device the player used last (decides autofocus on show).
  void used_gamepad(bool yes) { gamepad_last_ = yes; }
  // Last left stick position, so each axis event sees both axes.
  float stick[2] = {0.0f, 0.0f};

 private:
  struct Document {
    std::string path;
    Rml::ElementDocument* doc = nullptr;
  };
  class Listener final : public Rml::EventListener {
   public:
    explicit Listener(Ui& ui) : ui_(ui) {}
    void ProcessEvent(Rml::Event& event) override;

   private:
    Ui& ui_;
  };

  Document& document(const std::string& name);
  Rml::Element& element(const std::string& name, const std::string& id);
  Rml::ElementDocument* load(const std::string& path);
  // Handler id for the innermost element with a handler, from `element` up; 0 if none.
  std::uint32_t handler_for(Rml::Element* element) const;
  std::string name_of(const Rml::ElementDocument* doc) const;
  void focus_first(Rml::ElementDocument* doc);

  Library library_;
  RmlRenderer renderer_;
  Listener listener_{*this};
  Rml::Context* context_ = nullptr;
  std::string context_name_;
  std::map<std::string, Document, std::less<>> documents_;
  // (document, element id, event) -> handler id
  std::map<std::tuple<std::string, std::string, std::string>, std::uint32_t> handlers_;
  PadNav pad_;
  bool gamepad_last_ = false;
  std::uint32_t next_handler_ = 1;
  std::vector<std::uint32_t> events_;
  std::map<std::string, std::string> translations_;
  float scale_ = 1.0f;
  core::Insets safe_;
  bool high_contrast_ = false;
};

}  // namespace alpha::ui
