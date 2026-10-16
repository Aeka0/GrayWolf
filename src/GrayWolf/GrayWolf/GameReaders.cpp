#include "stdafx.h"

#include "GameReaders.h"

#include "CoDIWITranslator.h"
#include "CoDRawImageTranslator.h"
#include "SettingsManager.h"

namespace GameReaders
{
    std::unique_ptr<XImageDDS> TranslatePakImage(std::unique_ptr<uint8_t[]> Data, uint32_t Size, uint8_t Format, uint8_t MapType, uint32_t Width,
                                                 uint32_t Height, ImageUsageType Usage, uint8_t Levels)
    {
        if (Data == nullptr) return nullptr;

        // A2XY: the two 8-byte halves of each 16-byte block are stored swapped
        if (Format == 84)
        {
            auto Parts = (uint64_t*)Data.get();
            for (uint32_t Position = 0; Position + 1 < Size / 8; Position += 2)
                std::swap(Parts[Position], Parts[Position + 1]);
        }

        constexpr uint8_t MapTypeCube = 5;
        auto Result = CoDRawImageTranslator::TranslateBC(Data, Size, Width, Height, Format, Levels, MapType == MapTypeCube);
        if (Result != nullptr && Usage == ImageUsageType::NormalMap && SettingsManager::GetSetting("patchnormals", "true") == "true")
            Result->ImagePatchType = ImagePatch::Normal_Expand;
        return Result;
    }

    std::unique_ptr<XImageDDS> LoadIwdImage(const XImage_t& Image)
    {
        uint32_t ResultSize = 0;
        auto ImageData = CoDAssets::GamePackageCache->ExtractPackageObject(CoDAssets::GamePackageCache->HashPackageID(Image.ImageName), ResultSize);
        if (ImageData == nullptr) return nullptr;

        auto Result = CoDIWITranslator::TranslateIWI(ImageData, ResultSize);
        if (Result != nullptr && Image.ImageUsage == ImageUsageType::NormalMap && SettingsManager::GetSetting("patchnormals", "true") == "true")
            Result->ImagePatchType = ImagePatch::Normal_Bumpmap;
        return Result;
    }

    std::unique_ptr<XImageDDS> TranslateNormalPatched(const std::unique_ptr<uint8_t[]>& Data, uint32_t Size, uint32_t Width, uint32_t Height, uint8_t Format,
                                                      ImageUsageType Usage)
    {
        auto Result = CoDRawImageTranslator::TranslateBC(Data, Size, Width, Height, Format);
        if (Result != nullptr && Usage == ImageUsageType::NormalMap && SettingsManager::GetSetting("patchnormals", "true") == "true")
            Result->ImagePatchType = ImagePatch::Normal_Expand;
        return Result;
    }
}
