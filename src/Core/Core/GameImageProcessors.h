#pragma once

// Call of Duty doesn't store textures the way a renderer outside the game expects (normal
// maps keep 2 channels and rebuild the third; several maps pack 3 unrelated greyscale
// images into one RGB texture) - exported untouched they look wrong everywhere.
// Per-pixel port of Scobalula's GameImageUtil processors (GPL-3.0, upstream's `data/scripts`).
struct ImagePixel
{
    float R = 0.0f;
    float G = 0.0f;
    float B = 0.0f;
    float A = 0.0f;
};

class GameImageProcessors
{
public:

    // Rebuilds a normal map with X in alpha, Y in green (CoD's usual layout). Z is what's
    // left of the unit vector, clamped to zero rather than taking a root of a negative number.
    static ImagePixel UnpackNormalFromAlphaGreen(const ImagePixel& Packed);

    // The same, for maps that kept X in red and Y in green. BC5 stores two channels, so
    // this is the form that comes out of a BC5 compressed normal map.
    static ImagePixel UnpackNormalFromRedGreen(const ImagePixel& Packed);

    // Modern Warfare packs gloss (red), occlusion (blue), and a diamond-encoded normal
    // (green + alpha - rotated/scaled onto the centre diamond, not stored directly) into
    // one texture. Hands back the three maps it was hiding.
    static void UnpackNormalOcclusionGloss(const ImagePixel& Packed, ImagePixel& Normal, ImagePixel& Occlusion, ImagePixel& Gloss);

    // Splits a texture that carried colour in RGB and gloss in alpha into the two greyscale
    // and colour maps it really is
    static void SplitSpecularGloss(const ImagePixel& Packed, ImagePixel& Specular, ImagePixel& Gloss);

    // Splits a texture that is three unrelated greyscale maps packed one per channel. Both
    // the Modern Warfare 2 Remastered and the WWII textures are this shape.
    static void SplitChannels(const ImagePixel& Packed, ImagePixel& Red, ImagePixel& Green, ImagePixel& Blue);
};
