#include "stdafx.h"

#include "GameBlackOps.h"

#include "GameReaders.h"

namespace
{
    // Reads a material's blend mode off its GfxStateBits table (src/dst RGB blend factors, bits
    // 0-3/4-7 of the first state bits entry - the base pass). False if unreadable.
    bool ReadMaterialBlendBits(uint32_t MaterialPointer, uint32_t& OutSrcBlendRgb, uint32_t& OutDstBlendRgb)
    {
        auto MatData = CoDAssets::GameInstance->Read<BOXMaterial>(MaterialPointer);
        if (MatData.StateBitsCount == 0 || MatData.StateBitsTablePtr == 0)
        {
            return false;
        }

        auto Bits = CoDAssets::GameInstance->Read<BOXStateBits>(MatData.StateBitsTablePtr);
        OutSrcBlendRgb = Bits.LoadBits0 & 0xF;
        OutDstBlendRgb = (Bits.LoadBits0 >> 4) & 0xF;
        return true;
    }
}

// These read the zone the disk path serves as game memory (Games/T5/DiskGame.cpp)
std::unique_ptr<XAnim_t> GameBlackOps::ReadXAnim(const CoDAnim_t* Animation)
{
    return GameReaders::ReadAnim<BOXAnim, BOXAnimDeltaParts>(Animation, 2, { "viewmodel_", "t5_viewmodel_" });
}

std::unique_ptr<XModel_t> GameBlackOps::ReadXModel(const CoDModel_t* Model)
{
    return GameReaders::ReadModel<BOXModel, BOXModelSurface, uint32_t>(Model, 2, ReadXMaterial,
                                                                      [](XModel_t& Asset, const BOXModel& Data) { Asset.BoneInfoPtr = Data.BoneInfoPtr; });
}

const XMaterial_t GameBlackOps::ReadXMaterial(uint64_t MaterialPointer)
{
    auto MaterialData = CoDAssets::GameInstance->Read<BOXMaterial>(MaterialPointer);

    XMaterial_t Result(MaterialData.ImageCount);
    Result.MaterialName = FileSystems::GetFileNameWithoutExtension(CoDAssets::GameInstance->ReadNullTerminatedString(MaterialData.NamePtr));

    const auto UsageOf = [](uint32_t Semantic)
    {
        switch (Semantic)
        {
        case 0x25D709F9: // Detail Materials
        case 0x48FF11AE: // Face Materials
            return ImageUsageType::DiffuseMap;
        case 0x942CBFF0:
            return ImageUsageType::NormalMap;
        default:
            return GameReaders::SemanticUsage(Semantic);
        }
    };
    // Image name pointer: end of image - 8
    GameReaders::AddMaterialImages<BOXMaterialImage>(Result, MaterialData.ImageTablePtr, MaterialData.ImageCount,
                                                     [](uint64_t Image) { return GameReaders::ImageNameAt<uint32_t>(Image, sizeof(BOGfxImage) - 8); }, UsageOf);

    // The blend mode (src/dst RGB blend factors) as a material setting, exported to
    // "<name>_settings.txt". See ReadMaterialBlendBits() for the values.
    uint32_t SrcBlendRgb = 0, DstBlendRgb = 0;
    if (ReadMaterialBlendBits((uint32_t)MaterialPointer, SrcBlendRgb, DstBlendRgb))
    {
        uint32_t BlendBits[2] = { SrcBlendRgb, DstBlendRgb };
        Result.Settings.emplace_back("blendMode", "uint", BlendBits, 2);
    }

    return Result;
}

std::unique_ptr<XImageDDS> GameBlackOps::LoadXImage(const XImage_t& Image)
{
    return GameReaders::LoadIwdImage(Image);
}
