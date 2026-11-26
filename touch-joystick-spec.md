# Spec: Touch Joystick (stick virtual)

Status: proposta (não implementado).

## 1. Objetivo

Um joystick virtual na tela que se comporta como um **stick de gamepad**: alimenta `config.axes`
(e direções como botões), funciona junto com teclado/gamepad sem código extra, tem aparência
padrão e aceita **skins customizadas**. Só aparece e só lê input em **mobile**
(`ctx.platform.mobile`).

Princípios: declarado como dado em `config`, uma forma de fazer, determinístico e testável com
`simulate`, sem código de script por frame.

Inclui **botões de ação virtuais** (§3.1) pelo mesmo mecanismo: viram fontes de `config.actions`.

Fora de escopo na v1: gestos (pinch/swipe).

## 2. Pré-requisito: multitouch

Hoje só há um ponteiro (`ctx.input.mouse()`). Dois sticks ou stick + botão exigem rastrear dedos
por id. Núcleo: `core::Touches` (pura) alimentada por `SDL_EVENT_FINGER_*`; cada dedo é
"capturado" pelo primeiro consumidor (dev tools → UI → touch sticks → `Touch`/`MouseLeft`).
Um dedo capturado por um stick **não** gera `Touch`/`TouchLeft`/`MouseLeft`.

Exposto também ao script (útil e barato, mesmo seam):
```ts
ctx.input.touches(): { id: number; pos: Vec2; pressed: boolean; released: boolean }[]
// só dedos não capturados por UI/sticks; pos em game pixels (screen space)
```

## 3. Configuração

```ts
interface Config {
  touchSticks?: Record<string, TouchStick>; // nome -> stick; nomes viram fontes de input
}

interface TouchStick {
  zone: "left" | "right" | Rect;     // onde um toque ativa o stick; Rect em fração da safeArea [x,y,w,h] 0..1
  mode?: "floating" | "fixed";       // floating: nasce onde o dedo tocou (default); fixed: base parada
  anchor?: Vec2;                     // fixed: centro em fração da safeArea; default centro da zone
  radius?: number;                   // alcance do knob em game pixels; default 64
  deadzone?: number;                 // 0..0.9; default 0.15
  axes?: "both" | "x" | "y";         // default "both"
  snap?: 0 | 4 | 8;                  // quantiza direção (0 = analógico); default 0
  visible?: "always" | "touching";   // default "touching" em floating, "always" em fixed
  opacity?: number;                  // 0..1; default 0.6 (1 enquanto tocando)
  skin?: StickSkin;                  // omitido = aparência padrão
  player?: 1 | 2 | 3 | 4;            // default 1
}

interface StickSkin {
  base: string;          // nome de imagem em assets.images (ou frame "sheet:frame")
  knob: string;
  baseSize?: number;     // diâmetro em game pixels; default radius * 2
  knobSize?: number;     // default radius
  pressed?: { knob?: string; base?: string }; // frames trocados enquanto tocando
  tint?: string;         // cor multiplicada, "#rrggbbaa"
}
```

### Exemplo

```js
export const config = {
  touchSticks: {
    move: { zone: "left" },                                  // skin padrão
    aim:  { zone: "right", mode: "fixed",
            skin: { base: "ui/stick_base", knob: "ui/stick_knob", tint: "#ffd27fcc" } },
  },
  axes: {
    moveX: ["A/D", "GamepadLeftX", "TouchMoveX"],
    moveY: ["W/S", "GamepadLeftY", "TouchMoveY"],
    aimX:  ["GamepadRightX", "TouchAimX"],
    aimY:  ["GamepadRightY", "TouchAimY"],
  },
  actions: { fire: ["Space", "GamepadRT", "TouchAimActive"] },
};
```

Nenhum código no `update`: o jogo lê `ctx.input.axis("moveX")` como já faz.

### 3.1 Botões de ação

```ts
interface Config {
  touchButtons?: Record<string, TouchButton>; // nome -> botão; vira fonte Touch<Name>
}

interface TouchButton {
  pos: Vec2;                 // centro em fração da safeArea, 0..1
  radius?: number;           // área de toque em game pixels; default 40
  hitSlop?: number;          // margem extra de toque em px; default 8
  label?: string;            // texto na aparência padrão (ex.: "A", "⚡"); usa i18n se for chave
  visible?: "always" | "never"; // "never" = zona invisível; default "always"
  opacity?: number;          // default 0.6 (1 enquanto pressionado)
  skin?: ButtonSkin;         // omitido = aparência padrão (círculo + label)
  player?: 1 | 2 | 3 | 4;
}

interface ButtonSkin {
  up: string;                // imagem/frame em repouso
  down?: string;             // pressionado; default = up escurecido
  size?: number;             // diâmetro; default radius * 2
  tint?: string;
}
```

```js
export const config = {
  touchSticks:  { move: { zone: "left" } },
  touchButtons: {
    jump: { pos: [0.88, 0.80], label: "A" },
    dash: { pos: [0.75, 0.88], skin: { up: "ui/btn_dash", down: "ui/btn_dash_down" } },
  },
  actions: {
    jump: ["Space", "GamepadA", "TouchJump"],
    dash: ["Shift", "GamepadB", "TouchDash"],
  },
};
// ctx.input.pressed("jump") funciona igual para teclado, pad e touch.
```

Comportamento:
- Fonte `Touch<Name>` é um botão: `pressed`/`down`/`released` iguais aos de teclado e pad.
- Captura: dedo novo dentro de `radius + hitSlop` → botão fica `down` até esse dedo soltar
  (arrastar para fora **não** solta; evita soltar sem querer no meio da ação).
- Vários botões e sticks ao mesmo tempo (um dedo cada) — o caso stick esquerdo + pular/atirar.
- Prioridade de captura: game UI → botões → sticks (botão dentro da zone de um stick vence).
- Mesmas regras de "só mobile", pausa (solta tudo), fixed step e camadas de desenho dos sticks.
- Botões sobrepostos: erro de config (ambíguo), diferente de zones de sticks.

## 4. Fontes de input geradas

Para um stick `name` (PascalCase no nome da fonte: `move` → `Move`):

| Fonte | Tipo | Valor |
|---|---|---|
| `Touch<Name>X`, `Touch<Name>Y` | axis | -1..1 após deadzone/snap; Y+ = para baixo (igual a `GamepadLeftY`) |
| `Touch<Name>X-` / `X+` / `Y-` / `Y+` | botão | direção além de 0.5 (igual a `GamepadLeftX+`) |
| `Touch<Name>Active` | botão | dedo no stick |
| `Touch<Name>` (de `touchButtons`) | botão | dedo no botão |

Nomes de sticks e botões compartilham o espaço `Touch<Name>`: nome repetido é erro de config.

- Mesma regra de hoje: o axis lê a fonte de maior magnitude → teclado, pad e touch coexistem.
- `ctx.input.rebind` aceita essas fontes como qualquer outra.

## 5. Comportamento

- **Só mobile:** se `ctx.platform.mobile` é false, os sticks não existem: não desenham, não
  capturam dedos, as fontes leem 0. Em desktop com `--platform android` aparecem (preview e testes).
- **Ativação:** dedo novo dentro da `zone` e não consumido pela UI → captura. `floating`: base nasce
  no ponto do toque (clampada para caber na safeArea). Um dedo por stick; extras vão para frente.
- **Vetor:** `v = (pos - centro) / radius`, magnitude limitada a 1; abaixo da deadzone → 0;
  acima, remapeado `(|v| - dz) / (1 - dz)`; depois `axes` e `snap`.
- **Soltar:** knob volta ao centro; valores 0 no mesmo step; `floating` some se `visible:"touching"`.
- **Ordem de input:** dev tools → game UI (`ui::consumes`) → touch sticks → demais (Touch/Mouse).
  UI sobre a zone vence (botão de pausa num canto continua clicável).
- **Pausa / perda de foco:** sticks soltam todos os dedos e zeram.
- **Fixed step:** posições amostradas no início do step, como o resto do input.
- **Coordenadas:** zone/anchor relativas a `ctx.safeArea`; desenho em screen space, acima do mundo
  e dos efeitos, abaixo da game UI (RmlUi).

## 6. Aparência

- **Padrão (sem `skin`):** base = círculo com borda, knob = círculo preenchido, cinza neutro
  semitransparente; desenhado pelo renderer (sem asset), nítido em qualquer resolução
  (`core::PixelGrid`).
- **Skin:** imagens do pack (`assets.images` / sheets), filtragem conforme o `pixelMode` de cada
  imagem; `pressed` troca frames enquanto tocando; `tint` multiplica; `opacity` aplica por cima.
- Imagem inexistente → erro de config (ver §7), não fallback silencioso.

## 7. Validação e erros (no load do config)

- `config.touchSticks.move.zone: expected "left"|"right"|[x,y,w,h] in 0..1, got "top"`
- `config.touchSticks.aim.skin.knob: image "ui/stick_knb" not in assets.images; did you mean "ui/stick_knob"?`
- `config.touchSticks.move.deadzone: must be 0..0.9, got 1.2`
- `config.axes.moveX: unknown source "TouchMovX"; touch stick sources: TouchMoveX, TouchMoveY, ...`
- `config.touchSticks: at most kMaxTouchSticks (4) sticks, got 5`
- Zones sobrepostas: aviso (não erro); o primeiro na ordem do objeto ganha o dedo.

## 8. Testes

- `simulate(game, { platform: "android", touches: [...] })` com gestos por frame:
  ```js
  touches: [{ id: 1, path: [[0, [80, 600]], [10, [140, 600]], [40, null]] }] // frame, pos | null = solta
  ```
  Exercita o stick ponta a ponta (captura, vetor, deadzone, axis).
- Atalho já existente continua valendo: `inputs: { axes: { moveX: [[0, 1]] } }` (não passa pelo stick).
- Nativos (doctest, `src/input/`/`core`): matemática do vetor, deadzone, snap, captura por zone,
  prioridade UI > stick, liberação na pausa, desktop = inerte.
- Screenshot test com `--platform android` para skin padrão e skin custom.

## 9. Arquitetura

**Tudo em C++.** Captura de dedos, vetor/deadzone/snap, fontes de axis, desenho e skins vivem no
engine; nenhum JS embutido (nem módulo `.js`) participa. O script só declara `config.touchSticks`
e lê `ctx.input.axis(...)`; não há custo de script por frame.

```
core/touches        pura: dedos por id, captura por consumidor
core/touch_stick    pura: config validado + estado -> valores de axis/botões
core/touch_button   pura: hit test + estado down/pressed/released por dedo
platform/app_event  SDL_EVENT_FINGER_* -> core::Touches
script/config       parse/validação de config.touchSticks; registro das fontes Touch<Name>*
render/touch_stick_draw  aparência padrão e skins (screen space)
```
- Limites em `core/limits.hpp`: `kMaxTouchSticks = 4`, `kMaxTouchButtons = 12`, `kMaxTouches = 10`.
- Feature de jogo → vai no shipping (nada atrás de flags). Overlay de debug dos dedos só em `ALPHA_DEVTOOLS`.
- MCP: `game_state.input.touchSticks` (valor e dedo de cada stick).

## 10. Fases

1. Multitouch em `core::Touches` + `ctx.input.touches()` + `simulate({ touches })`.
2. `config.touchSticks` com aparência padrão, fontes de axis/botão, só mobile.
3. Skins (imagens, sheets, `pressed`, `tint`, `opacity`).
4. Docs: `docs/guide/` (input), `alpha-sdk/README.md`, `alpha.d.ts`, `alpha-sdk/llm/input.md`,
   GLOSSARY, site regenerado; usar no `examples/survivors` (move com stick).
5. `config.touchButtons` (aparência padrão + skins), usados no survivors para a ação especial.

(Fases 4 e 5 podem trocar de ordem; docs cobrem as duas antes de fechar.)

Cada fase termina com `python tools/gate.py` verde.

## 11. Questões em aberto

- `floating` como default ou `fixed`? (floating é o padrão de mercado em mobile)
- Mostrar em tablets/desktop com touchscreen (Windows touch)? Proposta v1: só `mobile`.
- Haptics ao ativar (reaproveitar `rumble`)?
