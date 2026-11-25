#include "host_fixture.hpp"

#include <fstream>

namespace alpha::test {

namespace fs = std::filesystem;

Sandbox::Sandbox() {
  fs::remove_all(dir);
  fs::create_directories(dir);
}

Sandbox::~Sandbox() { fs::remove_all(dir); }

std::string Sandbox::write(const std::string& name, const std::string& source) const {
  std::ofstream(dir / name) << source;
  return (dir / name).string();
}

script::SavePathFor Sandbox::save() const {
  return [path = save_file()](const std::string&) { return path; };
}

std::string Sandbox::save_file() const { return (dir / "save.json").string(); }

core::InputFrame space() {
  core::InputFrame frame;
  frame.players[0].held.set(*core::parse_input("Space"));
  return frame;
}

std::string ui_text(script::Host& host, const std::string& id) {
  if (!host.ui()) return "";
  for (const auto& [name, view] : host.ui()->view()) {
    if (view.shown && view.texts.contains(id)) return view.texts.at(id);
  }
  return "";
}

std::string rml(const std::vector<std::string>& ids) {
  std::string body;
  for (const std::string& id : ids) body += "<p id=\"" + id + "\"></p>";
  return "<rml><head></head><body>" + body + "</body></rml>";
}

std::string tiny(const std::string& hook, const std::string& body) {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const play = scene('play', { " + hook + "(ctx) { " + body + " } });\n"
         "export default game({ config: { title: 't', size: [288, 100], seed: 1,\n"
         "  actions: { jump: ['Space'] } }, start: 'play', scenes: { play },\n"
         "  assets: { sounds: {} } });\n";
}

std::string game_with(const std::string& config, const std::string& scene) {
  return "import { define, vec } from 'alpha'; const { scene, game } = define();\n"
         "const check = (ok, what) => { if (!ok) throw new Error('failed: ' + what); };\n"
         "const near = (a, b, eps = 0.5) => Math.abs(a - b) <= eps;\n"
         "const play = scene('play', { " + scene + " });\n"
         "export default game({ config: { title: 't', size: [400, 300], seed: 1, actions: {} " + config + " },\n"
         "  start: 'play', scenes: { play } });\n";
}

}  // namespace alpha::test
