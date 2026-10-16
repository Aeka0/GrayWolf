#pragma once

// A MapScene as a Cast scene (dtzxporter/cast), for Maya and Cast's Blender importer: <map>.cast
// holds the world geometry (one mesh per surface), its materials as Cast slots and one Instance per
// placement, each referring to models/<model>.cast (written once per model, in its own space).
// Units as the model files': centimetres unless the unit setting is Inch; Z up, as the game.
// Lights and the sky's look have no Cast node: the .glb keeps them (the map_cast setting adds this
// scene beside it).

#include "MapScene.h"

#include <string>

namespace Materials { class IAdapter; }

namespace Maps
{
    struct CastStats
    {
        size_t WorldMeshes = 0, Models = 0, Instances = 0, InstancesWithoutModel = 0, Materials = 0;
    };
    bool WriteCastScene(const MapScene& Scene, const std::string& CastPath, const Materials::IAdapter* Adapter,
                        CastStats& Stats, std::string& Error);
    // The quaternion (x, y, z, w) of a rotation given as rows: the local X, Y and Z axes in world space
    void QuatFromAxis(const float Axis[9], float Q[4]);
}
