#pragma once

#define WRAITHX_BUILD_MAJOR 1
#define WRAITHX_BUILD_MINOR 0
#define WRAITHX_BUILD_EXPONENT 102

class WraithX
{
public:
    // COM, and the image API (DirectXTex WIC) unless told otherwise
    static bool InitializeAPI(bool ImageAPI = true);
    static void ShutdownAPI(bool ImageAPI = true);
};