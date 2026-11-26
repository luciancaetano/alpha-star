# Spec: `alpha:net` — clientes TCP, UDP e WebSocket

Status: proposta (não implementado). Escopo v1: **apenas cliente**.

## 1. Objetivo

Dar aos jogos uma forma pequena e previsível de falar com servidores via TCP, UDP e WebSocket,
coerente com os princípios do engine: uma forma de fazer cada coisa, erros escritos para o agente,
script nunca derruba o engine, simulação em fixed step.

Fora de escopo na v1: servidor/listen, HTTP(S) genérico, descoberta LAN, NAT traversal,
`wss://` (TLS — fase 4).

## 2. Superfície da API

```ts
declare module "alpha:net" {
  export const net: {
    tcp(host: string, port: number, opts?: TcpOptions): Socket;
    udp(host: string, port: number, opts?: UdpOptions): Socket;
    ws(url: string, opts?: WsOptions): Socket;
  };

  export type Framing = "lines" | "u16" | "u32" | "raw";

  export interface TcpOptions {
    framing?: Framing;          // default "u32"
    maxMessageBytes?: number;   // default e teto: kMaxNetMessageBytes
    connectTimeout?: number;    // segundos; default 10
    noDelay?: boolean;          // TCP_NODELAY; default true
  }
  export interface UdpOptions {
    maxMessageBytes?: number;   // default e teto: kMaxUdpDatagramBytes (1200)
  }
  export interface WsOptions {
    protocols?: string[];       // Sec-WebSocket-Protocol
    headers?: Record<string, string>; // handshake; nomes reservados rejeitados
    maxMessageBytes?: number;
    connectTimeout?: number;
  }

  export type SocketState = "connecting" | "open" | "closing" | "closed";

  export interface Message {
    data: string | Uint8Array;  // string se texto (WS text / framing "lines"), senão Uint8Array
    step: number;               // step em que foi entregue
  }

  export interface Socket {
    readonly kind: "tcp" | "udp" | "ws";
    readonly state: SocketState;
    readonly error: string | null;   // preenchido quando state === "closed" por falha
    readonly closeCode: number | null; // WS: código de fechamento; tcp/udp: null
    readonly stats: { sent: number; received: number; queuedOut: number; queuedIn: number }; // bytes
    send(data: string | Uint8Array): boolean; // false = fila de saída cheia (nada enviado)
    receive(): Message[];                      // drena a fila de entrada deste step
    close(code?: number, reason?: string): void; // idempotente
  }
}
```

### Exemplo

```js
import { net } from "alpha:net";

const ws = net.ws("ws://localhost:9000/game");

export default function main(ctx) {
  ctx.onUpdate = () => {
    for (const m of ws.receive()) applyState(JSON.parse(m.data));
    if (ws.state === "open") ws.send(JSON.stringify({ x: player.pos.x }));
    if (ws.state === "closed") showError(ws.error);
  };
}
```

## 3. Semântica

### 3.1 Modelo de execução
- Criar um socket nunca bloqueia; começa em `"connecting"` (UDP vai direto para `"open"`).
- I/O real acontece fora do script (thread de rede ou polling não bloqueante no main loop).
- **No início de cada fixed step** o engine move o que chegou para a fila de entrada do socket
  e atualiza `state`/`error`. Durante o step tudo é imutável → ordem estável e reproduzível.
- `receive()` retorna e esvazia a fila; chamar duas vezes no mesmo step retorna `[]` na segunda.
- `send()` enfileira; o envio físico ocorre após o step. Em `"connecting"` também enfileira
  (enviado ao abrir). Em `"closing"/"closed"` retorna `false`.
- Sem callbacks: nenhum código de script roda fora de `onUpdate`/hooks existentes.

### 3.2 Transições de estado
```
connecting ──ok──> open ──close()/peer──> closing ──> closed
     └──falha/timeout─────────────────────────────────> closed (error != null)
```
- `closed` é terminal. Reconectar = criar novo socket.
- `close()` local limpo deixa `error === null`.

### 3.3 Por transporte
| | TCP | UDP | WS |
|---|---|---|---|
| Unidade | mensagem via `framing` | datagrama | mensagem WS |
| Ordem / entrega | garantida | nenhuma (pode perder/duplicar/reordenar) | garantida |
| `"connecting"` | handshake TCP | não existe | TCP + handshake HTTP |
| Texto vs binário | `"lines"` → string; demais → Uint8Array | Uint8Array (string enviada vira UTF-8) | conforme opcode |

Framing TCP:
- `"lines"`: UTF-8 terminado em `\n` (o `\n` é removido; `\r\n` aceito).
- `"u16"` / `"u32"`: prefixo big-endian com o tamanho do payload.
- `"raw"`: cada chunk recebido vira uma `Message` (sem garantia de fronteira).

WS: ping/pong e fragmentação tratados pelo engine; `send(string)` = frame texto,
`send(Uint8Array)` = frame binário. Só `ws://` na v1; `wss://` falha com erro explicativo.

### 3.4 Ciclo de vida
- O engine registra todo socket criado. Hot reload, fim de `alpha test`, quit e erro fatal fecham
  todos (antes de `SDL_Quit`).
- Pausa (`pauseOnFocusLoss`, background): conexões seguem abertas e a fila de entrada acumula até
  o limite; estourando, o socket fecha com erro (ver §5). No Android, se o SO derrubar a conexão,
  aparece como `closed` + `error`.

## 4. Limites (`core/limits.hpp`)

| Constante | Valor proposto | Significado |
|---|---|---|
| `kMaxNetSockets` | 32 | sockets abertos simultâneos |
| `kMaxNetMessageBytes` | 1 MiB | mensagem TCP/WS |
| `kMaxUdpDatagramBytes` | 1200 | datagrama UDP (seguro p/ MTU) |
| `kMaxNetQueueBytes` | 8 MiB | fila de entrada e de saída, por socket |
| `kMaxNetHeaderBytes` | 8 KiB | headers extras do handshake WS |

Opções podem reduzir, nunca aumentar os limites.

## 5. Erros (escritos para o agente)

Validação na fronteira (lança `TypeError`/`RangeError` síncrono, pausa o jogo como qualquer erro):
- `net.tcp: port must be an integer 1..65535, got 70000`
- `net.ws: url must start with ws:// (wss:// not supported yet), got "http://x"`
- `net.tcp: framing must be one of lines|u16|u32|raw, got "json"`
- `net.tcp: too many open sockets (32, limit kMaxNetSockets); close unused ones`
- `socket.send: message of 70000 bytes exceeds maxMessageBytes 65536`

Falhas de rede (assíncronas → `state = "closed"`, `error` preenchido, sem exceção):
- `connect to example.com:7777 refused`
- `connect to example.com:7777 timed out after 10 s`
- `dns lookup failed for "exmple.com"`
- `ws handshake failed: server answered HTTP 404`
- `peer sent 2 MiB message, over maxMessageBytes 1 MiB`
- `inbound queue full (8 MiB) while game was paused`

## 6. Testes e determinismo

- `alpha test` / `--test`: o backend nativo de `alpha:net` é trocado por um **fake em memória**.
  API de teste (em `alpha/test`, só `ALPHA_DEBUG_TOOLS`):
  ```js
  import { netFake } from "alpha/test";
  const srv = netFake.server("ws", "ws://localhost:9000/game", { latencySteps: 2, loss: 0 });
  srv.onConnect(peer => peer.send('{"hello":1}'));
  srv.messages();      // o que o jogo enviou
  ```
  Latência/perda/reordenação (UDP) usam o RNG seedado do jogo → testes não flaky.
- `--net-record PATH` (debug): grava mensagens entregues por step; `--net-replay PATH` as reproduz
  sem rede → bug de rede reproduzível.
- MCP: `game_state.net` lista sockets (kind, endpoint, state, error, stats).

## 7. Arquitetura nativa

```
src/net/            puro: Socket (estado, filas, framing, parser WS, limites) — sem SDL/JS
src/platform/net_*  backend real via SDL3_net (TCP/UDP, DNS); WS = handshake + RFC 6455 sobre TCP
script/net_bindings validação de fronteira, módulo embutido alpha:net, registro p/ ciclo de vida
```
- Ordem no loop: `platform net poll` → `net::deliver(step)` → script → `net::flush()`.
- Flavors: `alpha:net` vai para shipping. Fake, record/replay e `game_state.net` só com
  `ALPHA_DEBUG_TOOLS` / `ALPHA_MCP`.
- Android: `INTERNET` no manifest do export (só se o pack importar `alpha:net`, se viável).
- Notices: rodar `tools/make_notices.py` ao adicionar SDL3_net (e lib TLS na fase 4).

## 8. Fases

1. **Núcleo puro + fake**: `src/net/` (estados, filas, framing, limites) e `netFake`; TDD com doctest.
2. **TCP + UDP** reais via SDL3_net; bindings; `alpha.d.ts`.
3. **WebSocket `ws://`**: handshake, frames, ping/pong, close codes.
4. **TLS / `wss://`**: escolher mbedTLS vs TLS do SO medindo tamanho do binário.
5. **Docs + exemplo**: guide, `alpha-sdk/README.md`, `alpha.d.ts`, `alpha-sdk/llm/net.md`,
   GLOSSARY, site regenerado; exemplo pequeno (chat ou ghost multiplayer) com testes via fake.

Cada fase termina com `python tools/gate.py` verde.

## 9. Questões em aberto

- Thread de rede dedicada vs polling não bloqueante no main thread (preferência: polling, mais simples, sem estado compartilhado).
- Encoding padrão de `framing` TCP: `"u32"` ou `"lines"`?
- Expor `net.ws` em web export futuro (mapear para WebSocket do browser; TCP/UDP indisponíveis lá).
- Política de permissão/consentimento de rede para jogos exportados.
