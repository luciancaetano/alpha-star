#include "script/net_bindings.hpp"

#include <algorithm>
#include <array>

#include "core/error.hpp"
#include "core/limits.hpp"
#include "net/hub.hpp"
#include "net/ws_handshake.hpp"
#include "script/js_value.hpp"
#include "script/native_guard.hpp"
#if ALPHA_DEBUG_TOOLS
#include "script/net_fake_bindings.hpp"
#endif

namespace alpha::script {

namespace {

net::Hub& hub_of(JSContext* ctx, ScriptState& s) {
  if (s.net) return *s.net;
  s.net = std::make_unique<net::Hub>(instances_of(ctx).net_links());
#if ALPHA_DEBUG_TOOLS
  instances_of(ctx).attach_tape(s);
#endif
  return *s.net;
}

net::Socket& socket_of(JSContext* ctx, ScriptState& s, JSValueConst id) {
  net::Socket* sock = hub_of(ctx, s).find(static_cast<std::uint32_t>(number_arg(ctx, id, "socket id")));
  if (!sock) throw core::Error("this socket is closed and was released");
  return *sock;
}

std::size_t size_opt(JSContext* ctx, const Value& opts, const char* name, std::size_t cap) {
  const double n = number_arg(ctx, opts.prop(name).get(), name);
  if (n < 1 || n > static_cast<double>(cap)) throw core::Error(std::string(name) + " must be 1.." + std::to_string(cap));
  return static_cast<std::size_t>(n);
}

// opts from alpha:net (already shaped there): headers as [[name, value], ...].
void ws_extras(JSContext* ctx, const Value& opts, net::Address& a) {
  a.protocols = strings_arg(ctx, opts.prop("protocols").get(), "protocols");
  const std::vector<std::string> flat = strings_arg(ctx, opts.prop("headers").get(), "headers");
  std::size_t bytes = 0;
  for (std::size_t i = 0; i + 1 < flat.size(); i += 2) {
    if (net::ws_reserved_header(flat[i])) throw core::Error("net.ws: header \"" + flat[i] + "\" is set by the engine");
    bytes += flat[i].size() + flat[i + 1].size() + 4;
    a.headers.emplace_back(flat[i], flat[i + 1]);
  }
  if (bytes > core::kMaxNetHeaderBytes) {
    throw core::Error("net.ws: headers take " + std::to_string(bytes) + " bytes, over " + net::size_text(core::kMaxNetHeaderBytes));
  }
}

net::Address address_of(JSContext* ctx, JSValueConst* argv) {
  const std::string kind = string_arg(ctx, argv[0], "kind");
  const Value opts(ctx, JS_DupValue(ctx, argv[3]));
  net::Address a;
  if (kind == "ws") {
    a = net::parse_ws_url(string_arg(ctx, argv[1], "url"));
    ws_extras(ctx, opts, a);
  } else {
    a.kind = kind == "udp" ? net::Kind::udp : net::Kind::tcp;
    a.host = string_arg(ctx, argv[1], "host");
    a.port = static_cast<int>(number_arg(ctx, argv[2], "port"));
    a.framing = net::parse_framing(string_arg(ctx, opts.prop("framing").get(), "framing")).value_or(net::Framing::u32);
  }
  const std::size_t cap = a.kind == net::Kind::udp ? core::kMaxUdpDatagramBytes : core::kMaxNetMessageBytes;
  a.max_message = std::min(size_opt(ctx, opts, "maxMessageBytes", cap), a.kind == net::Kind::tcp ? net::framing_cap(a.framing) : cap);
  a.connect_timeout = number_arg(ctx, opts.prop("connectTimeout").get(), "connectTimeout");
  return a;
}

JSValue net_open(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  return JS_NewUint32(ctx, hub_of(ctx, s).open(address_of(ctx, argv)));
}

JSValue net_send(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  net::Socket& sock = socket_of(ctx, s, argv[0]);
  return JS_NewBool(ctx, sock.send(message_arg(ctx, argv[1], "socket.send")));
}

JSValue net_receive(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const std::vector<net::Message> got = socket_of(ctx, s, argv[0]).receive();
  Value out(ctx, JS_NewArray(ctx));
  for (std::uint32_t i = 0; i < got.size(); ++i) {
    JSValue m = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, m, "data", new_message_data(ctx, got[i]));
    JS_SetPropertyStr(ctx, m, "step", JS_NewFloat64(ctx, static_cast<double>(got[i].step)));
    JS_SetPropertyUint32(ctx, out.get(), i, m);
  }
  return out.release();
}

JSValue stats_of(JSContext* ctx, const net::Stats& st) {
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "sent", JS_NewFloat64(ctx, static_cast<double>(st.sent)));
  JS_SetPropertyStr(ctx, o, "received", JS_NewFloat64(ctx, static_cast<double>(st.received)));
  JS_SetPropertyStr(ctx, o, "queuedOut", JS_NewFloat64(ctx, static_cast<double>(st.queued_out)));
  JS_SetPropertyStr(ctx, o, "queuedIn", JS_NewFloat64(ctx, static_cast<double>(st.queued_in)));
  return o;
}

// { state, error, closeCode, stats } or null once released.
JSValue net_info(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  const net::Socket* sock = hub_of(ctx, s).find(static_cast<std::uint32_t>(number_arg(ctx, argv[0], "socket id")));
  if (!sock) return JS_NULL;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "state", JS_NewString(ctx, net::state_name(sock->state())));
  JS_SetPropertyStr(ctx, o, "error", sock->error().empty() ? JS_NULL : JS_NewString(ctx, sock->error().c_str()));
  JS_SetPropertyStr(ctx, o, "closeCode", sock->close_code() ? JS_NewInt32(ctx, *sock->close_code()) : JS_NULL);
  JS_SetPropertyStr(ctx, o, "stats", stats_of(ctx, sock->stats()));
  return o;
}

JSValue net_close(JSContext* ctx, ScriptState& s, JSValueConst* argv) {
  net::Socket* sock = hub_of(ctx, s).find(static_cast<std::uint32_t>(number_arg(ctx, argv[0], "socket id")));
  if (sock) sock->close(static_cast<int>(number_arg(ctx, argv[1], "close code")), string_arg(ctx, argv[2], "close reason"));
  return JS_UNDEFINED;
}

JSValue net_deliver(JSContext*, ScriptState& s, JSValueConst*) {
  if (s.net) s.net->deliver();
  return JS_UNDEFINED;
}

JSValue net_flush(JSContext*, ScriptState& s, JSValueConst*) {
  if (s.net) s.net->flush();
  return JS_UNDEFINED;
}

constexpr std::array<Native, 7> kNatives{{
    {"netOpen", guarded<net_open>, 4},
    {"netSend", guarded<net_send>, 2},
    {"netReceive", guarded<net_receive>, 1},
    {"netInfo", guarded<net_info>, 1},
    {"netClose", guarded<net_close>, 3},
    {"netDeliver", guarded<net_deliver>, 0},
    {"netFlush", guarded<net_flush>, 0},
}};

}  // namespace

net::Message message_arg(JSContext* ctx, JSValueConst v, const char* what) {
  if (JS_IsString(v)) return {string_arg(ctx, v, what), true};
  std::size_t size = 0;
  const std::uint8_t* bytes = JS_GetUint8Array(ctx, &size, v);
  if (!bytes) {
    JS_FreeValue(ctx, JS_GetException(ctx));
    throw core::Error(std::string(what) + ": data must be a string or a Uint8Array, got " + describe(ctx, v));
  }
  return {std::string(reinterpret_cast<const char*>(bytes), size), false};
}

JSValue new_message_data(JSContext* ctx, const net::Message& m) {
  if (m.text) return JS_NewStringLen(ctx, m.data.data(), m.data.size());
  return JS_NewUint8ArrayCopy(ctx, reinterpret_cast<const std::uint8_t*>(m.data.data()), m.data.size());
}

void install_net_bindings(JSContext* ctx, JSValueConst target) {
  install(ctx, target, kNatives);
#if ALPHA_DEBUG_TOOLS
  install_net_fake_bindings(ctx, target);
#endif
}

}  // namespace alpha::script
