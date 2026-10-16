#include "stdafx.h"

#include "HuskyMath.h"
#include "VectorMath.h"

#include <cstring>

float HuskyMath::HalfToFloat(uint16_t Half)
{
    // binary16 is one sign bit, five exponent bits, ten mantissa bits
    const uint32_t Sign = (uint32_t)(Half >> 15) & 0x1;
    const uint32_t Exponent = (uint32_t)(Half >> 10) & 0x1F;
    const uint32_t Mantissa = (uint32_t)Half & 0x3FF;

    uint32_t Bits = 0;

    if (Exponent == 0)
    {
        if (Mantissa == 0)
        {
            // Signed zero
            Bits = Sign << 31;
        }
        else
        {
            // Subnormal. Normalise it by shifting the mantissa up until the implicit bit
            // appears, taking the exponent down as we go.
            uint32_t Shifted = Mantissa;
            int32_t Adjust = -1;

            do
            {
                Shifted <<= 1;
                Adjust++;
            } while ((Shifted & 0x400) == 0);

            // 127 - 15 is the bias difference between binary32 and binary16
            const uint32_t NewExponent = (uint32_t)(127 - 15 - Adjust);
            Bits = (Sign << 31) | (NewExponent << 23) | ((Shifted & 0x3FF) << 13);
        }
    }
    else if (Exponent == 0x1F)
    {
        // Infinity or NaN, the mantissa carries which
        Bits = (Sign << 31) | (0xFF << 23) | (Mantissa << 13);
    }
    else
    {
        Bits = (Sign << 31) | ((Exponent + (127 - 15)) << 23) | (Mantissa << 13);
    }

    float Result = 0.0f;
    std::memcpy(&Result, &Bits, sizeof(Result));
    return Result;
}

Vector3 HuskyMath::UnpackNormalScaled(uint32_t Packed)
{
    const auto Byte1 = (float)((Packed >> 0) & 0xFF);
    const auto Byte2 = (float)((Packed >> 8) & 0xFF);
    const auto Byte3 = (float)((Packed >> 16) & 0xFF);
    const auto Byte4 = (float)((Packed >> 24) & 0xFF);

    // The fourth byte carries a scale shared by the three axes
    const auto Scale = (float)((Byte4 + 192.0) / 32385.0);

    return Vector3(
        (float)((Byte1 - 127.0) * Scale),
        (float)((Byte2 - 127.0) * Scale),
        (float)((Byte3 - 127.0) * Scale));
}

Vector3 HuskyMath::UnpackNormalTenBit(uint32_t Packed)
{
    // Three ten bit fields, each spanning -1 to 1
    return Vector3(
        (float)(((Packed & 0x3FF) / 1023.0) * 2.0 - 1.0),
        (float)((((Packed >> 10) & 0x3FF) / 1023.0) * 2.0 - 1.0),
        (float)((((Packed >> 20) & 0x3FF) / 1023.0) * 2.0 - 1.0));
}
