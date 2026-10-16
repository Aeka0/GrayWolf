#pragma once

// The fx kind for Treyarch's FxEffectDef as Black Ops 1, 2 and 3 lay it out: an effect read from the
// relocated zone into JSON - its lifetimes and bounds, then each element (looping, one-shot,
// emission) with its type, flags, spawn and life timings, ranges, atlas, velocity and visual state
// samples, and what it draws or plays by name (materials, models, effects, sounds). Values are the
// stored ones (angles in radians, times in milliseconds, ranges as base and amplitude). The
// structs are the ones Greyhound's CoDEffectTranslator read (removed upstream
// in 2022, e66d8f7), their offsets checked against the zonekit tables. BO4's element (0x280) is
// another layout, not read; Cold War's (0x340) only as far as its tables say (EffectLayout).

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Treyarch
{
    // ColdWar: what its zonekit tables say of the element (0x340: its type at 0x328, its visuals at
    // 0, their count at 0x332, four effect references at 0x160) - the type, what it draws and the
    // effects it starts, no timing: the rest of the struct is not mapped.
    enum class EffectLayout { Bo1, Bo2, Bo3, ColdWar };

    // What the reader reads a zone through.
    struct EffectZone
    {
        const std::vector<uint8_t>* Image = nullptr;
        std::function<int64_t(uint64_t Address)> Off;            // zone offset, -1 outside
        std::function<std::string(uint64_t Address)> String;     // a string pointer's text, "" outside
        // the asset at a zone offset by name; unset: the string its first field points to
        std::function<std::string(int64_t Pos)> AssetName;
    };

    // The FxEffectDef at Pos as JSON text (Name its name); false when its elements are not in the zone.
    bool ReadEffect(const EffectZone& Z, EffectLayout L, int64_t Pos, const std::string& Name, std::string& Json);
}
