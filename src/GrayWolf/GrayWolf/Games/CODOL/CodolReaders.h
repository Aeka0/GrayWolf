#pragma once

// Call of Duty Online's live readers, run over a zone served by VirtualGameSession
// (Games/CODOL/DiskGame.cpp). 32-bit, IW4's shapes with CODOL's struct sizes; every offset is one
// the zonekit codol tables read (XModel 0x168, its LOD 0x38, XModelSurfs 0x30, XSurface 0x54,
// Material 0x68, GfxImage 0x34, XAnimParts 0x58: MW2's own layout).

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class CoDAnim_t;
class CoDModel_t;
struct XMaterial_t;
struct XModel_t;
struct XAnim_t;

namespace Codol
{
    // GfxImage: the GfxImageLoadDef pointer at 0, the name at 0x30
    constexpr int64_t ImageLoadDef = 0x0, ImageName = 0x30;
    // GfxImageLoadDef: levelCount u8 at 0, D3D format at 8, resourceSize at 0xc, the pixels from 0x10
    constexpr int64_t LoadDefFormat = 0x8, LoadDefSize = 0xc, LoadDefData = 0x10;

    // Where a streamed XModelSurfs' file comes from (the zone being served: main/models/<name> in
    // its install's archives, unwrapped); nullptr when none. Resets the files already served.
    using StreamedFile = std::function<bool(const std::string& Name, std::vector<uint8_t>& Data)>;
    void ServeStreamedSurfaces(StreamedFile Source);

    std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model);
    std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation);
    XMaterial_t ReadXMaterial(uint64_t MaterialPointer);
}
