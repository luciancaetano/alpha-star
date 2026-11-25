#include "mcp_fixture.hpp"

namespace alpha::test {

namespace {

// title: menu with a Start button -> play; play spawns a bird and logs what
// the agent's inputs did: "jump <players>" on press, "down" while held,
// "x <value>" for player 2's moveX axis.
constexpr const char* kGame = R"(
import { define } from 'alpha'; const { scene, game } = define();
const title = scene('title', {
  enter(ctx) {
    ctx.ui.show('menu');
    ctx.ui.on('menu', 'start', 'click', () => ctx.go('play'));
  },
});
const play = scene('play', {
  enter(ctx) { ctx.spawn({ tags: ['bird'], pos: [10, 20] }); },
  update(ctx) {
    if (ctx.input.pressed('jump')) console.log('jump ' + ctx.input.players());
    if (ctx.input.down('jump')) console.log('down');
    const x = ctx.input.player(2).axis('moveX');
    if (x !== 0) console.log('x ' + x);
  },
});
export default game({ config: { title: 't', size: [100, 100], seed: 1,
    actions: { jump: ['Space'] }, axes: { moveX: ['A/D'] } },
  start: 'title', scenes: { title, play },
  assets: { sounds: {}, ui: { menu: 'menu.rml' } } });
)";

constexpr const char* kMenu = R"(<rml><head><style>
body { width: 100%; height: 100%; background-color: #102030; }
button { display: block; width: 50px; height: 20px; }
</style></head><body><button id="start">Start</button></body></rml>)";

}  // namespace

McpGame::McpGame() {
  box.write("menu.rml", kMenu);
  host.load(box.write("game.js", kGame));
}

McpGame::~McpGame() {
  host.release_ui();
  SDL_DestroyRenderer(sdl);
  SDL_DestroySurface(surface);
}

std::string McpGame::logs() const {
  std::string out;
  for (std::size_t i = 0; i < host.log().size(); ++i) out += host.log().at(i).text + "\n";
  return out;
}

}  // namespace alpha::test
