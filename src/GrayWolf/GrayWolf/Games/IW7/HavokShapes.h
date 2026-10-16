#pragma once

// Infinite Warfare's collision: no clip map brushes, Havok's (hk_2014.2.5-r1, hknp) shapes in
// binary packfiles. The clip map's packfile holds the world's, MapEnts' the brush models' (and
// the trigger volumes'); each is a HavokPhysicsShapeList: an array of shapes, their names
// ("World Entity 0", "<prefab>.map:entity N") and the shape tags' records (the content flags at 0,
// 24 bytes each). A shape is an hknpCompressedMeshShape (triangles, quads and convex pieces whose
// vertices only are kept) or an hknpDynamicCompoundShape of hknpConvexPolytopeShapes (their
// planes kept). Havok's unit is 32 of the game's.

#include "MapScene.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace IW7Havok
{
    // One shape, in the game's units: its convex pieces as brushes (contents from their tag), its
    // triangles (three vertex numbers each, three floats per vertex) and each triangle's contents
    struct Shape
    {
        std::string Name;
        std::vector<Maps::Brush> Brushes;
        std::vector<float> Vertices;
        std::vector<uint32_t> Triangles;
        std::vector<uint32_t> TriangleContents;
    };

    // The shapes of the packfile at Data (Size bytes), a HavokPhysicsShapeList; false when it is
    // none. Counters says what could not be read (havok_*).
    bool ReadShapeList(const uint8_t* Data, size_t Size, std::vector<Shape>& Out, std::map<std::string, size_t>& Counters);

    // The planes of the convex hull of Points (three floats each) as a brush's sides (Normal .
    // p <= Dist inside); a face's points are those within Eps of its plane, its plane fitted to
    // them, an axis' when within 1/1000 of it. False when the points bound no volume.
    bool HullSides(const std::vector<float>& Points, float Eps, std::vector<Maps::BrushSide>& Out);
}
