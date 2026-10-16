#pragma once

// A level's collision (MapScene::Collision, from its clip map) as JSON, for tools working on where
// a player can go rather than on what is drawn (navigation meshes, top-down plans):
// <map>_collision.json beside the .glb with map_collision on. Format "graywolf-collision".

#include "MapScene.h"

#include <string>

namespace Maps
{
    bool WriteCollisionJson(const MapScene& Scene, const std::string& Path, std::string& Error);
}
