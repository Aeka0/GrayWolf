#include "stdafx.h"

#include "WraithX.h"

#include "Image.h"

bool WraithX::InitializeAPI(bool ImageAPI)
{
    if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED))) { return false; }

    if (ImageAPI)
        Image::InitializeImageAPI();

    return true;
}

void WraithX::ShutdownAPI(bool ImageAPI)
{
    if (ImageAPI)
        Image::ShutdownImageAPI();

    CoUninitialize();
}
