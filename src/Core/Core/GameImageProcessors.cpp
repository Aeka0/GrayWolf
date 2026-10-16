#include "stdafx.h"

#include "GameImageProcessors.h"

#include <cmath>

namespace
{
    // The channels come in as 0 to 1 and the vectors they encode run -1 to 1
    inline float ToSigned(float Value)
    {
        return Value * 2.0f - 1.0f;
    }

    inline float ToChannel(float Value)
    {
        return Value * 0.5f + 0.5f;
    }

    // Rebuilds a normal from the two kept components. Where they already reach unit length
    // there's no root for Z, so blue is written flat black (not the midpoint, matching the
    // original) to signal at a glance that this pixel has no usable Z.
    ImagePixel RebuildNormal(float X, float Y)
    {
        const auto Z = 1.0f - (X * X) - (Y * Y);

        ImagePixel Result;
        Result.R = ToChannel(X);
        Result.G = ToChannel(Y);
        Result.B = Z > 0.0f ? ToChannel(std::sqrt(Z)) : 0.0f;
        Result.A = 1.0f;
        return Result;
    }
}

ImagePixel GameImageProcessors::UnpackNormalFromAlphaGreen(const ImagePixel& Packed)
{
    return RebuildNormal(ToSigned(Packed.A), ToSigned(Packed.G));
}

ImagePixel GameImageProcessors::UnpackNormalFromRedGreen(const ImagePixel& Packed)
{
    return RebuildNormal(ToSigned(Packed.R), ToSigned(Packed.G));
}

void GameImageProcessors::UnpackNormalOcclusionGloss(const ImagePixel& Packed, ImagePixel& Normal, ImagePixel& Occlusion, ImagePixel& Gloss)
{
    // Gloss and occlusion are stored plainly, one channel each
    Gloss.R = Gloss.G = Gloss.B = Packed.R;
    Gloss.A = 1.0f;

    Occlusion.R = Occlusion.G = Occlusion.B = Packed.B;
    Occlusion.A = 1.0f;

    // The normal was rotated and scaled from the centre diamond onto the unit square to
    // use the space better, so undo that before normalising
    const auto VectorX = ToSigned(Packed.G);
    const auto VectorY = ToSigned(Packed.A);

    const auto X = (VectorX + VectorY) * 0.5f;
    const auto Y = (VectorX - VectorY) * 0.5f;
    const auto Z = 1.0f - std::fabs(X) - std::fabs(Y);

    const auto Length = std::sqrt(X * X + Y * Y + Z * Z);
    const auto Scale = Length > 0.0f ? 1.0f / Length : 0.0f;

    Normal.R = ToChannel(X * Scale);
    Normal.G = ToChannel(Y * Scale);
    Normal.B = ToChannel(Z * Scale);
    Normal.A = 1.0f;
}

void GameImageProcessors::SplitSpecularGloss(const ImagePixel& Packed, ImagePixel& Specular, ImagePixel& Gloss)
{
    Specular.R = Packed.R;
    Specular.G = Packed.G;
    Specular.B = Packed.B;
    Specular.A = 1.0f;

    Gloss.R = Gloss.G = Gloss.B = Packed.A;
    Gloss.A = 1.0f;
}

void GameImageProcessors::SplitChannels(const ImagePixel& Packed, ImagePixel& Red, ImagePixel& Green, ImagePixel& Blue)
{
    Red.R = Red.G = Red.B = Packed.R;
    Red.A = 1.0f;

    Green.R = Green.G = Green.B = Packed.G;
    Green.A = 1.0f;

    Blue.R = Blue.G = Blue.B = Packed.B;
    Blue.A = 1.0f;
}
