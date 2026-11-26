#include "mcp/state_json.hpp"

#include <algorithm>

namespace alpha::mcp {

namespace {

std::string vec(core::Vec2 v) { return json::array({json::num(v.x), json::num(v.y)}); }

std::string entity(const ecs::EntityView& e) {
  std::vector<std::string> tags;
  for (const std::string& t : e.tags) tags.push_back(json::str(t));
  return json::object({{"id", json::num(e.id)},
                       {"tags", json::array(tags)},
                       {"pos", vec(e.pos)},
                       {"rotation", json::num(e.rotation)},
                       {"z", json::num(e.z)},
                       {"velocity", e.velocity ? vec(*e.velocity) : "null"},
                       {"animation", e.animation.empty() ? "null" : json::str(e.animation)},
                       {"layer", e.layer.empty() ? "null" : json::str(e.layer)},
                       {"alpha", json::num(e.alpha)},
                       {"text", e.text ? json::str(*e.text) : "null"},
                       {"particles", json::num(e.particles)},
                       {"data", e.data.empty() ? "null" : e.data}});
}

std::string document(const ui::DocumentView& d) {
  Fields texts;
  for (const auto& [id, text] : d.texts) texts.emplace_back(id, json::str(text));
  return json::object({{"shown", json::boolean(d.shown)}, {"texts", json::object(texts)}});
}

}  // namespace

Fields state_fields(const GameState& s) {
  std::vector<std::string> entities;
  for (const ecs::EntityView& e : s.entities) entities.push_back(entity(e));
  Fields docs;
  for (const auto& [name, d] : s.ui) docs.emplace_back(name, document(d));
  return {{"scene", json::str(s.scene)},
          {"paused", json::boolean(s.paused)},
          {"error", s.error.empty() ? "null" : json::str(s.error)},
          {"frame", json::num(static_cast<double>(s.frame))},
          {"fps", json::num(s.fps)},
          {"entities", json::array(entities)},
          {"joints", json::num(s.joints)},
          {"locale", s.locale.empty() ? "null" : json::str(s.locale)},
          {"music", s.music.empty() ? "null" : json::str(s.music)},
          {"appPaused", json::boolean(s.app_paused)},
          {"perf", s.perf.empty() ? "null" : s.perf},
          {"input", s.input.empty() ? "null" : s.input},
          {"net", s.net.empty() ? "[]" : s.net},
          {"ui", json::object(docs)}};
}

std::string input_json(const core::TouchPad& pad) {
  Fields sticks;
  for (std::size_t i = 0; i < pad.sticks().size(); ++i) {
    const core::StickState& st = pad.stick_states()[i];
    const std::string finger = st.finger ? json::num(static_cast<double>(*st.finger)) : "null";
    sticks.emplace_back(pad.sticks()[i].name, json::object({{"value", vec(st.value)}, {"finger", finger}}));
  }
  return json::object({{"touchSticks", json::object(sticks)}});
}

std::string net_json(const net::Hub* hub) {
  std::vector<std::string> sockets;
  if (!hub) return "[]";
  for (const auto& [id, s] : hub->sockets()) {
    const net::Stats st = s->stats();
    sockets.push_back(json::object(
        {{"id", json::num(id)},
         {"kind", json::str(net::kind_name(s->kind()))},
         {"endpoint", json::str(s->endpoint())},
         {"state", json::str(net::state_name(s->state()))},
         {"error", s->error().empty() ? "null" : json::str(s->error())},
         {"stats", json::object({{"sent", json::num(static_cast<double>(st.sent))},
                                 {"received", json::num(static_cast<double>(st.received))},
                                 {"queuedOut", json::num(static_cast<double>(st.queued_out))},
                                 {"queuedIn", json::num(static_cast<double>(st.queued_in))}})}}));
  }
  return json::array(sockets);
}

Fields log_fields(const core::LogRing& log, std::size_t since) {
  const std::size_t first = log.total() - log.size();  // number of the oldest kept line
  std::vector<std::string> lines;
  for (std::size_t n = std::max(since, first); n < log.total(); ++n) {
    const core::LogLine& line = log.at(n - first);
    lines.push_back(json::object({{"n", json::num(static_cast<double>(n))},
                                  {"level", json::str(core::level_name(line.level))},
                                  {"text", json::str(line.text)}}));
  }
  const std::size_t dropped = since < first ? first - since : 0;
  return {{"lines", json::array(lines)},
          {"next", json::num(static_cast<double>(log.total()))},
          {"dropped", json::num(static_cast<double>(dropped))}};
}

}  // namespace alpha::mcp
