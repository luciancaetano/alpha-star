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

// tiles.bmp: 2 tiles of 8x8 side by side; maps/tiles.tsj references it as ../tiles.bmp.
// level: 4x3 map; "ground" has the bottom row plus one block, "deco" one tile;
// "spawns" has a point "start" with a property.
void write_map(const Sandbox& box, const std::string& ground_extra = "") {
  SDL_Surface* px = SDL_CreateSurface(16, 8, SDL_PIXELFORMAT_RGBA32);
  SDL_SaveBMP(px, (box.dir / "tiles.bmp").string().c_str());
  SDL_DestroySurface(px);
  std::filesystem::create_directories(box.dir / "maps");
  box.write("maps/tiles.tsj", R"({ "name": "tiles", "image": "../tiles.bmp", "imagewidth": 16, "imageheight": 8,
    "tilewidth": 8, "tileheight": 8, "columns": 2, "tilecount": 2, "margin": 0, "spacing": 0 })");
  box.write("maps/level.tmj", R"({ "orientation": "orthogonal", "infinite": false, "width": 4, "height": 3,
    "tilewidth": 8, "tileheight": 8, "tilesets": [{ "firstgid": 1, "source": "tiles.tsj" }],
    "layers": [
      { "type": "tilelayer", "name": "ground", "width": 4, "height": 3, )" + ground_extra + R"(
        "data": [0, 0, 0, 0,  2, 0, 0, 0,  1, 1, 1, 1] },
      { "type": "tilelayer", "name": "deco", "width": 4, "height": 3, "data": [0, 0, 0, 2147483650, 0, 0, 0, 0, 0, 0, 0, 0] },
      { "type": "objectgroup", "name": "spawns", "objects": [
        { "id": 1, "name": "start", "type": "player", "x": 12, "y": 4, "point": true,
          "properties": [{ "name": "lives", "type": "int", "value": 3 }] }] }] })");
}

std::string game(const std::string& enter) {
  return "import { define } from 'alpha'; const { scene, game } = define();\n"
         "const check = (ok, what) => { if (!ok) throw new Error('failed: ' + what); };\n"
         "const play = scene('play', { enter(ctx) { " + enter + " } });\n"
         "export default game({ config: { title: 't', size: [100, 100], seed: 1, actions: {} },\n"
         "  start: 'play', scenes: { play }, assets: { maps: { level: 'maps/level.tmj' } } });\n";
}

std::string boot_error(const Sandbox& box, const std::string& enter) {
  Host host(box.save());
  host.load(box.write("g.js", game(enter)));
  return host.error();
}

}  // namespace

TEST_CASE("spawnMap draws each tile layer as one entity and merges collision cells into static boxes") {
  Sandbox box;
  write_map(box);
  Host host(box.save());
  host.load(box.write("g.js", game(R"(
    const map = ctx.spawnMap('level', { collide: ['ground'], z: -1 });
    check(map.size[0] === 32 && map.size[1] === 24 && map.tileSize[0] === 8, 'sizes');
    check(map.layers.join() === 'ground,deco,spawns', 'layers ' + map.layers);
    const walls = ctx.findAll('ground');
    check(walls.length === 2, 'merged boxes: ' + walls.length);
    check(walls[1].pos.x === 16 && walls[1].pos.y === 20, 'bottom row box at ' + walls[1].pos.x + ',' + walls[1].pos.y);
    const start = ctx.map('level').objects('spawns')[0];
    check(start.name === 'start' && start.type === 'player' && start.x === 12 && start.point, 'object');
    check(start.properties.lives === 3, 'properties');
  )")));
  REQUIRE_MESSAGE(!host.paused(), host.error());
  const auto shapes = host.frame().shapes;
  REQUIRE(shapes.size() == 2);  // ground and deco layers; colliders have no shape
  const auto& ground = *shapes[0].shape.tiles;
  CHECK(shapes[0].pos == Vec2{16, 12});
  REQUIRE(ground.tiles.size() == 5);
  CHECK(ground.tiles[0].pos == Vec2{0, 8});
  CHECK(ground.tiles[0].src == Region{8, 0, 8, 8});  // gid 2 = second tile
  CHECK(shapes[1].shape.tiles->tiles.at(0).src == Region{8, 0, 8, 8});  // flip bits masked
}

TEST_CASE("map mistakes list the maps and layers that exist") {
  Sandbox box;
  write_map(box);
  CHECK(boot_error(box, "ctx.spawnMap('level2');").find("spawnMap(\"level2\"): not in assets.maps; have: level") !=
        std::string::npos);
  CHECK(boot_error(box, "ctx.spawnMap('level', { collide: ['walls'] });")
            .find("no tile layer \"walls\"; it has: ground, deco") != std::string::npos);
  CHECK(boot_error(box, "ctx.map('level').objects('coins');").find("it has: spawns") != std::string::npos);
  write_map(box, R"("encoding": "base64", "compression": "zlib",)");
  CHECK(boot_error(box, "").find("set \"Tile Layer Format\" to CSV") != std::string::npos);
}
