#pragma once

// Black Ops 7's asset readers, over its zones served by the disk template (Games/BO7/DiskGame.cpp):
// models, materials, images. Every offset is one the zonekit tables read (zonekit/profiles/bo7,
// the loaders of cod.exe 0x6aadce0e) or one the data proves; C2M's BlackOps7.cs and BlackOps6.cs
// gave the names. There is no running-game reader: Black Ops 7 is read from its files only.

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "CoDXModelTranslator.h"

class GameBlackOps7
{
public:
    static std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model);
    static void LoadXModel(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel);
    static const XMaterial_t ReadXMaterial(uint64_t MaterialPointer);
    static std::unique_ptr<XImageDDS> LoadXImage(const XImage_t& Image);
    static std::unique_ptr<XImageDDS> ReadXImage(const CoDImage_t* Image);
    // sndasset (0x40, loader 0x38924d0): flags u8 0x30 (bits 4-5 not 0: its .xsub key at 8; else its
    // frames in the zone at 8, size u32 0x10), seek table size u32 0x20, frames' size u32 0x24, frame
    // count u32 0x28, rate u16 0x32 (Games/IW/IwctxReaders, shared with Black Ops 6)
    static std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound);
    static constexpr int64_t SoundKey = 8, SoundSeek = 0x20, SoundLoaded = 0x24, SoundFrames = 0x28, SoundFlags = 0x30, SoundRate = 0x32;

    // xanim (0x80, loader 0x38c21b0): Black Ops 6's packed keys in 1 to 4 .xsub parts
    // (Games/IW/IwctxReaders, XAnimLayout), read by the rules of the game's own sampler
    static std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation);
    static void LoadXAnim(const std::unique_ptr<XAnim_t>& Anim, std::unique_ptr<WraithAnim>& Result);
    // notetracks: named by the script strings of the zones opened (IwctxReaders::ScriptString)

    // An asset's name from its 63-bit hash (HashIWAsset: FNV-1a 64 from 0x47f5817a5ef961ba, lower
    // case), "<prefix>_<hex>" when no index knows it
    static std::string HashName(const char* Prefix, uint64_t Hash);
    static uint64_t HashIWAsset(const std::string& Name);

    // A model's bone names: FNV-1a 64 of the name; bone_<hex> when no index knows it
    static std::string BoneName(uint64_t Hash);
};
