#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>

#include "host_fixture.hpp"

using alpha::core::Region;
using alpha::core::Vec2;
using alpha::script::Host;
using alpha::test::Sandbox;

namespace {

// tiles.bmp: 3 tiles of 8x8 in a row; tile 1 animates [1, 2] at 2 fps.
// Level "Start" (4x3 cells of 8 px, LDtk lists the top layer first): Entities,
// Walls (IntGrid 1 wall / 2 oneway, one auto tile flipped x), Deco (one animated
// tile at half opacity). "Next" sits right of it in an external .ldtkl file.
constexpr const char* kProject = R"({ "jsonVersion": "1.5.3", "worldLayout": "Free", "externalLevels": true,
  "defs": {
    "tilesets": [{ "uid": 1, "identifier": "Tiles", "relPath": "tiles.bmp", "pxWid": 24, "pxHei": 8, "tileGridSize": 8,
      "spacing": 0, "padding": 0, "__cWid": 3, "__cHei": 1,
      "enumTags": [{ "enumValueId": "Spike", "tileIds": [2] }],
      "customData": [{ "tileId": 1, "data": "{\"frames\": [1, 2], \"fps\": 2}" }, { "tileId": 2, "data": "sharp" }] }],
    "layers": [{ "uid": 10, "identifier": "Walls", "type": "IntGrid",
      "intGridValues": [{ "value": 1, "identifier": "wall" }, { "value": 2, "identifier": "oneway" }] }],
    "entities": [{ "identifier": "Player" }, { "identifier": "Coin" }] },
  "levels": [
    { "identifier": "Start", "iid": "L0", "worldX": 0, "worldY": 0, "pxWid": 32, "pxHei": 24, "__bgColor": "#112233",
      "fieldInstances": [{ "__identifier": "music", "__type": "String", "__value": "cave" }],
      "__neighbours": [{ "levelIid": "L1", "dir": "e" }],
      "layerInstances": [
        { "__identifier": "Entities", "__type": "Entities", "__cWid": 4, "__cHei": 3, "__gridSize": 8, "iid": "E", "layerDefUid": 11,
          "entityInstances": [
            { "__identifier": "Player", "iid": "P", "px": [12, 8], "__grid": [1, 0], "__pivot": [0.5, 1], "width": 8, "height": 8,
              "__tags": ["hero"], "fieldInstances": [
                { "__identifier": "hp", "__type": "Int", "__value": 3 },
                { "__identifier": "target", "__type": "EntityRef", "__value": { "entityIid": "C", "layerIid": "E", "levelIid": "L0" } },
                { "__identifier": "spot", "__type": "Point", "__value": { "cx": 1, "cy": 1 } }] },
            { "__identifier": "Coin", "iid": "C", "px": [20, 4], "__grid": [2, 0], "__pivot": [0.5, 0.5], "width": 8, "height": 8 }] },
        { "__identifier": "Walls", "__type": "IntGrid", "__cWid": 4, "__cHei": 3, "__gridSize": 8, "iid": "W", "layerDefUid": 10,
          "__tilesetDefUid": 1, "intGridCsv": [0, 0, 0, 0, 2, 0, 0, 0, 1, 1, 1, 1],
          "autoLayerTiles": [{ "px": [0, 16], "src": [0, 0], "t": 0, "f": 1 }] },
        { "__identifier": "Deco", "__type": "Tiles", "__cWid": 4, "__cHei": 3, "__gridSize": 8, "iid": "D", "layerDefUid": 12,
          "__tilesetDefUid": 1, "__opacity": 1, "gridTiles": [{ "px": [24, 0], "src": [8, 0], "t": 1, "f": 0, "a": 0.5 }] }] },
    { "identifier": "Next", "iid": "L1", "worldX": 32, "worldY": 0, "pxWid": 32, "pxHei": 24, "externalRelPath": "world/Next.ldtkl" }] })";

constexpr const char* kNext = R"({ "identifier": "Next", "iid": "L1", "pxWid": 32, "pxHei": 24,
  "__neighbours": [{ "levelIid": "L0", "dir": "w" }], "layerInstances": [] })";

void write_world(const Sandbox& box, const std::string& project = kProject) {
  SDL_Surface* px = SDL_CreateSurface(24, 8, SDL_PIXELFORMAT_RGBA32);
  SDL_SaveBMP(px, (box.dir / "tiles.bmp").string().c_str());
  SDL_DestroySurface(px);
  std::filesystem::create_directories(box.dir / "world");
  box.write("w.ldtk", project);
  box.write("world/Next.ldtkl", kNext);
}

std::string game(const std::string& enter, const std::string& assets = "{ worlds: { w: 'w.ldtk' } }") {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const check = (ok, what) => { if (!ok) throw new Error('failed: ' + what); };\n"
         "const play = scene('play', { enter(ctx) { " + enter + " } });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 1, actions: {} },\n"
         "  start: 'play', scenes: { play }, assets: " + assets + " });\n";
}

std::string boot_error(const Sandbox& box, const std::string& enter, const std::string& assets = "{ worlds: { w: 'w.ldtk' } }") {
  Host host(box.save());
  host.load(box.write("g.js", game(enter, assets)));
  return host.error();
}

bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

const Region* deco_src(Host& host) {
  const auto shapes = host.frame().shapes;
  return shapes.size() < 2 || shapes[1].shape.tiles->tiles.empty() ? nullptr : &shapes[1].shape.tiles->tiles[0].src;
}

}  // namespace

TEST_CASE("spawnLevel draws background, tile layers with flips and opacity, and IntGrid colliders") {
  Sandbox box;
  write_world(box);
  Host host(box.save());
  host.load(box.write("g.js", game(R"(
    const lvl = ctx.spawnLevel('w', 'Start', { background: true, collide: { Walls: { wall: 'wall', 2: { tag: 'ledge', oneWay: true } } } });
    check(lvl.layers.join() === 'Deco,Walls,Entities', 'draw order ' + lvl.layers);
    check(ctx.findAll('wall').length === 1 && ctx.findAll('ledge').length === 1, 'merged colliders');
    const floor = ctx.findAll('wall')[0];
    check(floor.pos.x === 16 && floor.pos.y === 20, 'floor at ' + floor.pos.x + ',' + floor.pos.y);
  )")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const auto shapes = host.frame().shapes;
  REQUIRE(shapes.size() == 5);  // background, Deco still + animated, Walls still + animated
  CHECK(shapes[0].pos == Vec2{16, 12});
  CHECK(shapes[1].shape.tiles->tiles.empty());
  const auto& anim = shapes[2].shape.tiles->tiles.at(0);
  CHECK(anim.pos == Vec2{24, 0});
  CHECK(anim.alpha == doctest::Approx(0.5));
  const auto& wall = shapes[3].shape.tiles->tiles.at(0);
  CHECK(wall.pos == Vec2{0, 16});
  CHECK((wall.flip_x && !wall.flip_y));
}

TEST_CASE("animated tiles advance with scene time") {
  Sandbox box;
  write_world(box);
  Host host(box.save());
  host.load(box.write("g.js", game("ctx.spawnLevel('w', 'Start');")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  REQUIRE(deco_src(host) != nullptr);
  CHECK(*deco_src(host) == Region{8, 0, 8, 8});
  for (int i = 0; i < 36; ++i) host.step({}, 1.0f / 60.0f);
  CHECK(*deco_src(host) == Region{16, 0, 8, 8});
}

TEST_CASE("entities spawn through factories with typed fields in world pixels") {
  Sandbox box;
  write_world(box);
  Host host(box.save());
  host.load(box.write("g.js", game(R"(
    const lvl = ctx.spawnLevel('w', 'Start', { pos: [100, 0], entities: {
      Player: (e) => ({ tags: ['player'], pos: e.pos, data: { hp: e.fields.hp } }),
      Coin: (e) => [{ tags: ['coin'], pos: e.pos }] } });
    const p = lvl.entities('Player')[0];
    check(p.pos[0] === 112 && p.pos[1] === 8 && p.local[0] === 12, 'player pos ' + p.pos);
    check(p.fields.target.entity === 'C' && p.fields.spot.join() === '1,1' && p.tags[0] === 'hero', 'fields');
    check(lvl.entity(p.fields.target.entity) === ctx.find('coin'), 'entity ref resolves to the spawned coin');
    check(ctx.find('player').data.hp === 3 && lvl.entities().length === 2, 'factories');
    check(lvl.fields.music === 'cave', 'level fields');
  )")));
  CHECK_MESSAGE(!host.paused(), host.error());
}

TEST_CASE("live levels read and edit cells; colliders and tiles follow") {
  Sandbox box;
  write_world(box);
  Host host(box.save());
  host.load(box.write("g.js", game(R"(
    const lvl = ctx.spawnLevel('w', 'Start', { collide: { Walls: 'solid' } });
    check(lvl.int('Walls', 0, 1) === 2 && lvl.int('Walls', -1, 0) === 0, 'int reads (0 outside)');
    check(lvl.toCell([20, 17]).join() === '2,2' && lvl.toWorld([2, 2]).join() === '20,20', 'cell <-> world');
    lvl.setInt('Walls', 0, 1, 0);
    lvl.setInt('Walls', 3, 0, 'wall');
    check(ctx.findAll('solid').length === 2, 'colliders rebuilt: ' + ctx.findAll('solid').length);
    lvl.setTile('Deco', 1, 1, 2, { flipY: true });
    const t = lvl.tile('Deco', 1, 1);
    check(t.id === 2 && t.flipY && t.tags[0] === 'Spike' && t.data === 'sharp', 'tile info');
    lvl.setTile('Deco', 1, 1, null);
    check(lvl.tile('Deco', 1, 1) === null, 'tile cleared');
    lvl.destroy();
    check(ctx.findAll('solid').length === 0, 'destroy removes colliders');
  )")));
  CHECK_MESSAGE(!host.paused(), host.error());
}

TEST_CASE("world views list levels, neighbours, external levels and entities by iid") {
  Sandbox box;
  write_world(box);
  Host host(box.save());
  host.load(box.write("g.js", game(R"(
    const w = ctx.world('w');
    check(w.layout === 'Free' && w.levels.join() === 'Start,Next', 'levels');
    check(w.level('Start').neighbours[0].level === 'Next' && w.level('Start').neighbours[0].dir === 'e', 'neighbours');
    check(w.levelAt([40, 10]).id === 'Next' && w.levelAt([-5, 0]) === null, 'levelAt');
    check(w.level('Next').pos[0] === 32 && w.level('Next').layers.length === 0, 'external level');
    check(w.entity('C').level === 'Start' && w.entity('nope') === null, 'entity by iid');
    check(ctx.findAll('wall').length === 0, 'data views spawn nothing');
  )")));
  CHECK_MESSAGE(!host.paused(), host.error());
}

TEST_CASE("level mistakes name what exists") {
  Sandbox box;
  write_world(box);
  CHECK(has(boot_error(box, "ctx.spawnLevel('x', 'Start');"), "spawnLevel(\"x\"): not in assets.worlds; have: w"));
  CHECK(has(boot_error(box, "ctx.spawnLevel('w', 'Nope');"), "no level \"Nope\"; it has: Start, Next"));
  CHECK(has(boot_error(box, "ctx.spawnLevel('w', 'Start', { collide: { Deco: 'x' } });"),
            "no IntGrid layer \"Deco\"; it has: Walls"));
  CHECK(has(boot_error(box, "ctx.spawnLevel('w', 'Start', { collide: { Walls: { lava: 'x' } } });"),
            "no IntGrid value \"lava\"; it has: 0, 1 (wall), 2 (oneway)"));
  CHECK(has(boot_error(box, "ctx.spawnLevel('w', 'Start', { entities: { Enemy: () => null } });"),
            "no entity \"Enemy\" in the project; it has: Player, Coin"));
  CHECK(has(boot_error(box, "ctx.spawnLevel('w', 'Start').setInt('Walls', 9, 0, 1);"), "outside layer \"Walls\" (4 x 3 cells)"));
  CHECK(has(boot_error(box, "", "{ maps: { a: 'a.tmj' } }"), "replaced by assets.worlds"));
  write_world(box, R"({ "jsonVersion": "1.5.3", "levels": [], "worlds": [{ "identifier": "A" }] })");
  CHECK(has(boot_error(box, ""), "multi-world projects are not supported"));
}
