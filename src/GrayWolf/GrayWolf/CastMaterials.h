#pragma once

// A game's material description (MaterialDescription.h) as Cast's material slots, so a .cast file
// names every texture the game's adapter found, not only a colour, normal and specular map.

#include "MaterialDescription.h"

#include <vector>

struct CastMaterialSlots;
class Cast;

namespace Materials
{
    // albedo, normal, roughness (gloss when the map is a gloss map read inverted), metal, specular,
    // ao, emissive; the maps Cast has no slot for (a colour mask, a tint gradient, the layers)
    // follow as extra0, extra1... Paths as the description has them (relative to the model file).
    CastMaterialSlots CastSlots(const Description& D);
    std::vector<CastMaterialSlots> CastSlots(const std::vector<Description>& Materials);
}
