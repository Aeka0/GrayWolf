#pragma once

#include <cstdint>
#include <string>

// IWI images of the "version 6" layout (CoD4, WaW; CoD2's version 5 is the same) as a DDS file in
// memory: every format of that version, the wavelet-compressed ones included (CoD2: 25 images).
// Header "IWi", version, u8 format, u8 flags, u16 width height depth, u32 fileSizeForPicmip[4];
// then the mips smallest first, each holding all its faces. Each size the header gives is
// checked; a wavelet stream must decode within the file. The DDS lists the largest mip first;
// a cube map's faces each with all their mips, as DDS readers expect.
namespace CoDIWIDecoder
{
    bool ToDds(const uint8_t* Data, size_t Size, std::string& Dds, std::string& Error);
}
