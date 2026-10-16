#pragma once

// Black Ops Cold War's collision (clip_map) as Maps::Collision. Not Quake's clipMap_t: the
// collision is a list of partitions, the first the world's, partition N the brush model "*N" of
// the entity string (an entity keeps its number, Treyarch::EntityText), each holding its own
// brushes, triangles and oriented boxes. Layout in ClipMap.cpp, measured on the zones' bytes.

#include "MapCollision.h"

#include <cstdint>
#include <map>
#include <string>

namespace ColdWar
{
    // Fills Out from the clip_map asset at Clip; Counters says what could not be read
    // (collision_*). False when it holds nothing.
    bool ReadClipMap(Maps::Collision& Out, const Maps::ClipSource& Src, int64_t Clip, std::map<std::string, size_t>& Counters);

    // Cold War's content flags in Quake's lineage, as the other games keep them (Maps::Collision):
    // the same bits but the trigger's (0x2000000 here, 0x40000000 there); 0x40000000 (depthPrime)
    // and 0x80000000 (transparent), which mean something else there, dropped.
    uint32_t LineageContents(uint32_t Contents);
}
