#pragma once

#include <cstdint>

class Vector3;

// Vertex-packing math ported from Scobalula's Husky (GPL-3.0), the utility half every
// game-specific reader decodes through. Pure functions, so checkable without a game running.
class HuskyMath
{
public:

    // Expands an IEEE 754 binary16 to a float; implemented from the standard (not
    // transliterated bit-twiddling) so it's checkable against the format's defined values.
    static float HalfToFloat(uint16_t Half);

    // Unpacks a vertex normal stored as three offset bytes and a shared scale.
    // Used by World at War, Black Ops and Black Ops 2.
    static Vector3 UnpackNormalScaled(uint32_t Packed);

    // Unpacks a vertex normal stored as three ten bit fields.
    // Used by Ghosts, Advanced Warfare and Modern Warfare Remastered.
    static Vector3 UnpackNormalTenBit(uint32_t Packed);
};
