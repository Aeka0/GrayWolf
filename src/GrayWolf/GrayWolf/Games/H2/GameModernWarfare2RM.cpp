#include "stdafx.h"

#include "GameModernWarfare2RM.h"

#include "CoDAssets.h"
#include "GameReaders.h"

#include "Strings.h"
#include "FileSystems.h"
#include "SettingsManager.h"

#include <format>

#pragma pack(push, 1)
struct MW2RXModelLod
{
    float LodDistance;

    uint16_t NumSurfs;
    uint16_t SurfacesIndex;

    uint8_t Padding[40];

    uint64_t SurfsPtr;

    uint8_t Padding2[16];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct MW2RXModel
{
    uint64_t NamePtr;
    uint8_t NumBones;
    uint8_t NumRootBones;
    uint8_t NumSurfaces;
    uint8_t LodRampType;

    uint8_t Padding[0x2C];

    uint64_t BoneIDsPtr;
    uint64_t ParentListPtr;
    uint64_t RotationsPtr;
    uint64_t TranslationsPtr;
    uint64_t PartClassificationPtr;
    uint64_t BaseMatriciesPtr;
    uint64_t UnknownPtr;
    uint64_t MaterialHandlesPtr;

    MW2RXModelLod ModelLods[6];

    uint8_t NumLods;
    uint8_t MaxLods;

    uint8_t Padding2[0x8E];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct MW2RXMaterial
{
    uint64_t NamePtr;

    uint8_t Padding[0x124];

    uint8_t ImageCount;

    uint8_t Padding2[19];

    uint64_t ImageTablePtr;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct MW2RSoundAliasEntry
{
    uint64_t NamePtr;
    uint8_t Padding[0x18];
    uint64_t FileSpecPtr;
    uint8_t Padding2[0xD8];
};
#pragma pack(pop)

bool GameModernWarfare2RM::LoadAssets()
{
    bool NeedsAnims = (SettingsManager::GetSetting("showxanim", "true") == "true");
    bool NeedsModels = (SettingsManager::GetSetting("showxmodel", "true") == "true");
    bool NeedsImages = (SettingsManager::GetSetting("showximage", "false") == "true");
    bool NeedsSounds = (SettingsManager::GetSetting("showxsounds", "false") == "true");

    if (NeedsAnims)
    {
        // First pool; skip the 8-byte free-head pointer
        auto AnimationOffset = CoDAssets::GameOffsetInfos[0] + 8;
        auto AnimationCount = CoDAssets::GamePoolSizes[0];

        auto MaximumPoolOffset = (AnimationCount * sizeof(MWRXAnim)) + AnimationOffset;
        auto MinimumPoolOffset = CoDAssets::GameOffsetInfos[0];

        MWRXAnim PlaceholderAnim = {};

        for (uint32_t i = 0; i < AnimationCount; i++)
        {
            auto AnimResult = CoDAssets::GameInstance->Read<MWRXAnim>(AnimationOffset);

            // Free slot: a null handle, or one pointing inside the pool
            if ((AnimResult.NamePtr > MinimumPoolOffset && AnimResult.NamePtr < MaximumPoolOffset) || AnimResult.NamePtr == 0)
            {
                AnimationOffset += sizeof(MWRXAnim);
                continue;
            }

            auto AnimName = CoDAssets::GameInstance->ReadNullTerminatedString(AnimResult.NamePtr);

            auto LoadedAnim = new CoDAnim_t();
            LoadedAnim->AssetName = AnimName;
            LoadedAnim->AssetPointer = AnimationOffset;
            LoadedAnim->Framerate = AnimResult.Framerate;
            LoadedAnim->FrameCount = AnimResult.NumFrames;
            LoadedAnim->BoneCount = AnimResult.TotalBoneCount;

            // "void" is the base xanim
            if (AnimName == "void")
            {
                PlaceholderAnim = AnimResult;
                LoadedAnim->AssetStatus = WraithAssetStatus::Placeholder;
            }
            else if (AnimResult.BoneIDsPtr == PlaceholderAnim.BoneIDsPtr && AnimResult.DataBytePtr == PlaceholderAnim.DataBytePtr && AnimResult.DataShortPtr == PlaceholderAnim.DataShortPtr && AnimResult.DataIntPtr == PlaceholderAnim.DataIntPtr && AnimResult.RandomDataBytePtr == PlaceholderAnim.RandomDataBytePtr && AnimResult.RandomDataIntPtr == PlaceholderAnim.RandomDataIntPtr && AnimResult.RandomDataShortPtr == PlaceholderAnim.RandomDataShortPtr && AnimResult.NotificationsPtr == PlaceholderAnim.NotificationsPtr && AnimResult.DeltaPartsPtr == PlaceholderAnim.DeltaPartsPtr)
            {
                LoadedAnim->AssetStatus = WraithAssetStatus::Placeholder;
            }
            else
            {
                LoadedAnim->AssetStatus = WraithAssetStatus::Loaded;
            }

            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedAnim);

            AnimationOffset += sizeof(MWRXAnim);
        }
    }

    if (NeedsModels)
    {
        // Second pool; skip the 8-byte free-head pointer
        auto ModelOffset = CoDAssets::GameOffsetInfos[1] + 8;
        auto ModelCount = CoDAssets::GamePoolSizes[1];

        auto MaximumPoolOffset = (ModelCount * sizeof(MW2RXModel)) + ModelOffset;
        auto MinimumPoolOffset = CoDAssets::GameOffsetInfos[1];

        MW2RXModel PlaceholderModel = {};

        for (uint32_t i = 0; i < ModelCount; i++)
        {
            auto ModelResult = CoDAssets::GameInstance->Read<MW2RXModel>(ModelOffset);

            // Free slot: a null handle, or one pointing inside the pool
            if ((ModelResult.NamePtr > MinimumPoolOffset && ModelResult.NamePtr < MaximumPoolOffset) || ModelResult.NamePtr == 0)
            {
                ModelOffset += sizeof(MW2RXModel);
                continue;
            }

            auto ModelName = FileSystems::GetFileName(CoDAssets::GameInstance->ReadNullTerminatedString(ModelResult.NamePtr));

            auto LoadedModel = new CoDModel_t();
            LoadedModel->AssetName = ModelName;
            LoadedModel->AssetPointer = ModelOffset;
            LoadedModel->BoneCount = ModelResult.NumBones;
            LoadedModel->LodCount = ModelResult.NumLods;

            // "void" is the base xmodel
            if (ModelName == "void")
            {
                PlaceholderModel = ModelResult;
                LoadedModel->AssetStatus = WraithAssetStatus::Placeholder;
            }
            else if ((ModelResult.BoneIDsPtr == PlaceholderModel.BoneIDsPtr && ModelResult.ParentListPtr == PlaceholderModel.ParentListPtr && ModelResult.RotationsPtr == PlaceholderModel.RotationsPtr && ModelResult.TranslationsPtr == PlaceholderModel.TranslationsPtr && ModelResult.PartClassificationPtr == PlaceholderModel.PartClassificationPtr && ModelResult.BaseMatriciesPtr == PlaceholderModel.BaseMatriciesPtr && ModelResult.NumLods == PlaceholderModel.NumLods && ModelResult.MaterialHandlesPtr == PlaceholderModel.MaterialHandlesPtr && ModelResult.NumBones == PlaceholderModel.NumBones) || ModelResult.NumLods == 0)
            {
                // Set as placeholder, data matches void, or the model has no lods
                LoadedModel->AssetStatus = WraithAssetStatus::Placeholder;
            }
            else
            {
                LoadedModel->AssetStatus = WraithAssetStatus::Loaded;
            }

            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedModel);

            ModelOffset += sizeof(MW2RXModel);
        }
    }

    if (NeedsImages)
    {
        // Third pool; skip the 8-byte free-head pointer
        auto ImageOffset = CoDAssets::GameOffsetInfos[2] + 8;
        auto ImageCount = CoDAssets::GamePoolSizes[2];

        auto MaximumPoolOffset = (ImageCount * sizeof(MWRGfxImage)) + ImageOffset;
        auto MinimumPoolOffset = CoDAssets::GameOffsetInfos[2];

        for (uint32_t i = 0; i < ImageCount; i++)
        {
            auto ImageResult = CoDAssets::GameInstance->Read<MWRGfxImage>(ImageOffset);

            // Free slot: a null handle, or one pointing inside the pool
            if ((ImageResult.NextHead > MinimumPoolOffset && ImageResult.NextHead < MaximumPoolOffset) || ImageResult.NamePtr == 0)
            {
                ImageOffset += sizeof(MWRGfxImage);
                continue;
            }

            auto ImageName = FileSystems::GetFileName(CoDAssets::GameInstance->ReadNullTerminatedString(ImageResult.NamePtr));

            if (ImageResult.Streamed > 0)
            {
                uint32_t LargestMip = 0;
                uint32_t LargestWidth = ImageResult.Width;
                uint32_t LargestHeight = ImageResult.Height;

                for (uint32_t i = 0; i < 3; i++)
                {
                    if (ImageResult.MipLevels[i].Width > LargestWidth)
                    {
                        LargestMip = (i + 1);
                        LargestWidth = ImageResult.MipLevels[i].Width;
                        LargestHeight = ImageResult.MipLevels[i].Height;
                    }
                }

                auto LoadedImage = new CoDImage_t();
                LoadedImage->AssetName = ImageName;
                LoadedImage->AssetPointer = ImageOffset;
                LoadedImage->Width = (uint16_t)LargestWidth;
                LoadedImage->Height = (uint16_t)LargestHeight;
                LoadedImage->Format = ImageResult.ImageFormat;
                LoadedImage->AssetStatus = WraithAssetStatus::Loaded;

                CoDAssets::GameAssets->LoadedAssets.push_back(LoadedImage);
            }

            ImageOffset += sizeof(MWRGfxImage);
        }
    }

    if (NeedsSounds)
    {
        // A temporary table for duplicates, since we are tracing from alias entries...
        std::set<uint64_t> UniqueEntries;

        // Fourth pool; skip the 8-byte free-head pointer
        auto LoadedSoundOffset = CoDAssets::GameOffsetInfos[3] + 8;
        auto LoadedSoundCount = CoDAssets::GamePoolSizes[3];

        auto MaximumPoolOffset = (LoadedSoundCount * sizeof(MWRSoundAlias)) + LoadedSoundOffset;
        auto MinimumPoolOffset = CoDAssets::GameOffsetInfos[3];

        for (uint32_t i = 0; i < LoadedSoundCount; i++)
        {
            auto SoundResult = CoDAssets::GameInstance->Read<MWRSoundAlias>(LoadedSoundOffset);

            // Free slot: a null handle, or one pointing inside the pool
            if ((SoundResult.NamePtr > MinimumPoolOffset && SoundResult.NamePtr < MaximumPoolOffset) || SoundResult.NamePtr == 0)
            {
                LoadedSoundOffset += sizeof(MWRSoundAlias);
                continue;
            }

            auto SoundName = CoDAssets::GameInstance->ReadNullTerminatedString(SoundResult.NamePtr);

            for (uint32_t j = 0; j < SoundResult.EntryCount; j++)
            {
                auto SoundAliasEntry = CoDAssets::GameInstance->Read<MW2RSoundAliasEntry>(SoundResult.EntriesPtr + (j * sizeof(MW2RSoundAliasEntry)));

                auto SoundFileSpec = CoDAssets::GameInstance->Read<MWRSoundAliasFileSpec>(SoundAliasEntry.FileSpecPtr);
                if (SoundFileSpec.Type == 1)
                {
                    auto LoadedSoundPtr = CoDAssets::GameInstance->Read<uint64_t>(SoundAliasEntry.FileSpecPtr + 8);
                    if (UniqueEntries.insert(LoadedSoundPtr).second == false)
                        continue;
                    auto LoadedSoundInfo = CoDAssets::GameInstance->Read<AWLoadedSound>(LoadedSoundPtr);

                    auto LoadedSoundName = CoDAssets::GameInstance->ReadNullTerminatedString(LoadedSoundInfo.NamePtr);

                    auto LoadedSound = new CoDSound_t();
                    LoadedSound->AssetName = FileSystems::GetFileNameWithoutExtension(LoadedSoundName);
                    LoadedSound->AssetPointer = LoadedSoundInfo.SoundDataPtr;
                    LoadedSound->FrameRate = LoadedSoundInfo.FrameRate;
                    LoadedSound->FrameCount = LoadedSoundInfo.FrameCount;
                    LoadedSound->AssetSize = LoadedSoundInfo.SoundDataSize;
                    LoadedSound->ChannelsCount = LoadedSoundInfo.Channels;
                    LoadedSound->IsFileEntry = false;
                    LoadedSound->FullPath = FileSystems::GetDirectoryName(LoadedSoundName);
                    LoadedSound->DataType = (LoadedSoundInfo.Format == 7 || LoadedSoundInfo.Format == 6) ? SoundDataTypes::FLAC_WithHeader : SoundDataTypes::WAV_NeedsHeader;
                    LoadedSound->AssetStatus = WraithAssetStatus::Loaded;
                    LoadedSound->Length = (uint32_t)(1000.0f * (float)(LoadedSound->FrameCount / (float)(LoadedSound->FrameRate)));
                    CoDAssets::GameAssets->LoadedAssets.push_back(LoadedSound);
                }
                else
                {
#if _DEBUG
                    printf("Unknown sound type: %d\n", SoundFileSpec.Type);
#endif
                }
            }
            LoadedSoundOffset += sizeof(MWRSoundAlias);
        }
    }

    return true;
}

bool GameModernWarfare2RM::LoadAssetsPS()
{
    bool NeedsAnims = (SettingsManager::GetSetting("showxanim", "true") == "true");
    bool NeedsModels = (SettingsManager::GetSetting("showxmodel", "true") == "true");
    bool NeedsImages = (SettingsManager::GetSetting("showximage", "false") == "true");
    bool NeedsSounds = (SettingsManager::GetSetting("showxsounds", "false") == "true");

    if (NeedsAnims)
    {
        auto XAnimPool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 5 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(XAnimPool.Root, CoDAssets::ParasyteRequest, [](ps::XAsset64& Asset)
        {
            auto AnimResult = CoDAssets::GameInstance->Read<MWRXAnim>(Asset.Header);
            auto AnimName = CoDAssets::GameInstance->ReadNullTerminatedString(AnimResult.NamePtr);

            auto LoadedAnim = new CoDAnim_t();
            LoadedAnim->AssetName = AnimName;
            LoadedAnim->AssetPointer = Asset.Header;
            LoadedAnim->Framerate = AnimResult.Framerate;
            LoadedAnim->FrameCount = AnimResult.NumFrames;
            LoadedAnim->BoneCount = AnimResult.TotalBoneCount;
            LoadedAnim->AssetStatus = Asset.Temp == 1 ? WraithAssetStatus::Placeholder : WraithAssetStatus::Loaded;
            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedAnim);
        });
    }

    if (NeedsModels)
    {
        auto Pool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 7 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(Pool.Root, CoDAssets::ParasyteRequest, [](ps::XAsset64& Asset)
        {
            auto ModelResult = CoDAssets::GameInstance->Read<MW2RXModel>(Asset.Header);
            auto ModelName = FileSystems::GetFileName(CoDAssets::GameInstance->ReadNullTerminatedString(ModelResult.NamePtr));
            auto LoadedModel = new CoDModel_t();
            LoadedModel->AssetName = ModelName;
            LoadedModel->AssetPointer = Asset.Header;
            LoadedModel->BoneCount = ModelResult.NumBones;
            LoadedModel->LodCount = ModelResult.NumLods;
            LoadedModel->AssetStatus = (Asset.Temp == 1 || ModelResult.NumLods == 0) ? WraithAssetStatus::Placeholder : WraithAssetStatus::Loaded;
            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedModel);
        });
    }

    if (NeedsImages)
    {
        auto XAnimPool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 16 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(XAnimPool.Root, CoDAssets::ParasyteRequest, [](ps::XAsset64& Asset)
        {
            auto ImageResult = CoDAssets::GameInstance->Read<MWRGfxImage>(Asset.Header);
            auto ImageName = FileSystems::GetFileName(CoDAssets::GameInstance->ReadNullTerminatedString(ImageResult.NamePtr));

            uint32_t LargestWidth = ImageResult.Width;
            uint32_t LargestHeight = ImageResult.Height;

            if (ImageResult.Streamed > 0)
            {
                for (uint32_t i = 0; i < 3; i++)
                {
                    if (ImageResult.MipLevels[i].Width > LargestWidth)
                    {
                        LargestWidth = ImageResult.MipLevels[i].Width;
                        LargestHeight = ImageResult.MipLevels[i].Height;
                    }
                }
            }

            auto LoadedImage = new CoDImage_t();
            LoadedImage->AssetName = ImageName;
            LoadedImage->AssetPointer = Asset.Header;
            LoadedImage->Width = (uint16_t)LargestWidth;
            LoadedImage->Height = (uint16_t)LargestHeight;
            LoadedImage->Format = ImageResult.ImageFormat;
            LoadedImage->AssetStatus = WraithAssetStatus::Loaded;
            LoadedImage->Streamed = ImageResult.Streamed > 0;

            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedImage);
        });
    }

    if (NeedsSounds)
    {
        // A temporary table for duplicates, since we are tracing from alias entries...
        std::set<uint64_t> UniqueEntries;
        auto Pool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 17 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(Pool.Root, CoDAssets::ParasyteRequest, [&UniqueEntries](ps::XAsset64& Asset)
        {
            auto SoundResult = CoDAssets::GameInstance->Read<MWRSoundAlias>(Asset.Header);
            auto SoundName = CoDAssets::GameInstance->ReadNullTerminatedString(SoundResult.NamePtr);

            for (uint32_t j = 0; j < SoundResult.EntryCount; j++)
            {
                auto SoundAliasEntry = CoDAssets::GameInstance->Read<MW2RSoundAliasEntry>(SoundResult.EntriesPtr + (j * sizeof(MW2RSoundAliasEntry)));

                auto SoundFileSpec = CoDAssets::GameInstance->Read<MWRSoundAliasFileSpec>(SoundAliasEntry.FileSpecPtr);
                if (SoundFileSpec.Type == 1)
                {
                    auto LoadedSoundPtr = CoDAssets::GameInstance->Read<uint64_t>(SoundAliasEntry.FileSpecPtr + 8);
                    if (UniqueEntries.insert(LoadedSoundPtr).second == false)
                        continue;
                    auto LoadedSoundInfo = CoDAssets::GameInstance->Read<AWLoadedSound>(LoadedSoundPtr);

                    auto LoadedSoundName = CoDAssets::GameInstance->ReadNullTerminatedString(LoadedSoundInfo.NamePtr);

                    auto LoadedSound = new CoDSound_t();
                    LoadedSound->AssetName = FileSystems::GetFileNameWithoutExtension(LoadedSoundName);
                    LoadedSound->AssetPointer = LoadedSoundInfo.SoundDataPtr;
                    LoadedSound->FrameRate = LoadedSoundInfo.FrameRate;
                    LoadedSound->FrameCount = LoadedSoundInfo.FrameCount;
                    LoadedSound->AssetSize = LoadedSoundInfo.SoundDataSize;
                    LoadedSound->ChannelsCount = LoadedSoundInfo.Channels;
                    LoadedSound->IsFileEntry = false;
                    LoadedSound->FullPath = FileSystems::GetDirectoryName(LoadedSoundName);
                    LoadedSound->DataType = (LoadedSoundInfo.Format == 7 || LoadedSoundInfo.Format == 6) ? SoundDataTypes::FLAC_WithHeader : SoundDataTypes::WAV_NeedsHeader;
                    LoadedSound->AssetStatus = WraithAssetStatus::Loaded;
                    LoadedSound->Length = (uint32_t)(1000.0f * (float)(LoadedSound->FrameCount / (float)(LoadedSound->FrameRate)));
                    CoDAssets::GameAssets->LoadedAssets.push_back(LoadedSound);
                }
                else
                {
#if _DEBUG
                    printf("Unknown sound type: %d\n", SoundFileSpec.Type);
#endif
                }
            }
        });
    }

    return true;
}

std::unique_ptr<XAnim_t> GameModernWarfare2RM::ReadXAnim(const CoDAnim_t* Animation)
{
    return GameReaders::ReadAnim<MWRXAnim, MWRXAnimDeltaParts>(Animation, 4, { "viewmodel_", "vm_", "h2_wpn", "h2_vm" });
}

std::unique_ptr<XModel_t> GameModernWarfare2RM::ReadXModel(const CoDModel_t* Model)
{
    return GameReaders::ReadModel<MW2RXModel, MWRXModelSurface, uint64_t>(Model, 4, ReadXMaterial);
}

std::unique_ptr<XImageDDS> GameModernWarfare2RM::ReadXImage(const CoDImage_t* Image)
{
    auto Usage = (Image->Format == 84) ? ImageUsageType::NormalMap : ImageUsageType::DiffuseMap;
    if (Usage == ImageUsageType::DiffuseMap)
    {
        if (Image->AssetName.ends_with("_n") || Image->AssetName.ends_with("_nml"))
        {
            Usage = ImageUsageType::NormalMap;
        }
    }
    if (ps::state != nullptr)
        return LoadXImagePS(XImage_t(Usage, 0, Image->AssetPointer, Image->AssetName));
    else
        return LoadXImage(XImage_t(Usage, 0, Image->AssetPointer, Image->AssetName));
}

struct XImageData
{
    uint16_t Locale;
    uint16_t PackageIndex;
    uint32_t Checksum;
    uint64_t Offset;
    uint64_t Size;
};

std::unique_ptr<XImageDDS> GameModernWarfare2RM::LoadXImagePS(const XImage_t& Image)
{
    // Only PAK images are supported
    size_t ResultSize = 0;

    auto ImageInfo = CoDAssets::GameInstance->Read<MWRGfxImage>(Image.ImagePtr);
    std::unique_ptr<uint8_t[]> ImageData = nullptr;
    uint32_t LargestWidth = 0;
    uint32_t LargestHeight = 0;

    if (ImageInfo.Streamed > 0)
    {
        uint32_t LargestMip = 0;

        for (uint32_t i = 0; i < 3; i++)
        {
            if (ImageInfo.MipLevels[i].Width > LargestWidth)
            {
                LargestMip = i + 1;
                LargestWidth = ImageInfo.MipLevels[i].Width;
                LargestHeight = ImageInfo.MipLevels[i].Height;
            }
        }

        auto ImageStreamInfo = CoDAssets::GameInstance->Read<XImageData>(Image.ImagePtr + sizeof(MWRGfxImage) + sizeof(XImageData) * LargestMip);
        auto ImagePackageName = std::format("imagefile{}.pak", ImageStreamInfo.PackageIndex);
        ImageData = CoDAssets::GamePackageCache->ExtractPackageObject(ImagePackageName, ImageStreamInfo.Offset, ImageStreamInfo.Size, ResultSize);
    }
    else
    {
        ResultSize = CoDAssets::GameInstance->Read<uint32_t>(Image.ImagePtr + 40);
        LargestWidth = (uint32_t)CoDAssets::GameInstance->Read<uint16_t>(Image.ImagePtr + 44);
        LargestHeight = (uint32_t)CoDAssets::GameInstance->Read<uint16_t>(Image.ImagePtr + 46);

        ImageData = std::make_unique<uint8_t[]>(ResultSize);

        if (CoDAssets::GameInstance->Read(ImageData.get(), CoDAssets::GameInstance->Read<uint64_t>(Image.ImagePtr + 56), ResultSize) != ResultSize)
            return nullptr;
    }

    return GameReaders::TranslatePakImage(std::move(ImageData), (uint32_t)ResultSize, ImageInfo.ImageFormat, ImageInfo.MapType, LargestWidth, LargestHeight, Image.ImageUsage);
}

const XMaterial_t GameModernWarfare2RM::ReadXMaterial(uint64_t MaterialPointer)
{
    auto MaterialData = CoDAssets::GameInstance->Read<MW2RXMaterial>(MaterialPointer);

    XMaterial_t Result(MaterialData.ImageCount);
    Result.MaterialName = FileSystems::GetFileNameWithoutExtension(CoDAssets::GameInstance->ReadNullTerminatedString(MaterialData.NamePtr));
    // Image name pointer: end of image - 8
    GameReaders::AddMaterialImages<MWRXMaterialImage>(Result, MaterialData.ImageTablePtr, MaterialData.ImageCount,
                                                      [](uint64_t Image) { return GameReaders::ImageNameAt<uint64_t>(Image, sizeof(MWRGfxImage) - 8); },
                                                      GameReaders::SemanticUsage);
    return Result;
}

std::unique_ptr<XImageDDS> GameModernWarfare2RM::LoadXImage(const XImage_t& Image)
{
    // Only PAK images are supported
    size_t ResultSize = 0;

    auto ImageInfo = CoDAssets::GameInstance->Read<MWRGfxImage>(Image.ImagePtr);

    if (ImageInfo.Streamed <= 0) { return nullptr; }

    uint32_t LargestWidth = 0, LargestHeight = 0;
    const uint32_t LargestMip = GameReaders::LargestLevel(ImageInfo, LargestWidth, LargestHeight);

    uint64_t PAKTableOffset = (((Image.ImagePtr - (CoDAssets::GameOffsetInfos[2] + 8)) / sizeof(MWRGfxImage)) * (sizeof(MWRPAKImageEntry) * 4)) + CoDAssets::GameOffsetInfos[5] + (LargestMip * sizeof(MWRPAKImageEntry));

    auto ImageStreamInfo = CoDAssets::GameInstance->Read<MWRPAKImageEntry>(PAKTableOffset);

    auto ImagePackageName = CoDAssets::GameInstance->ReadNullTerminatedString(ImageStreamInfo.ImagePAKInfoPtr + 0x18);

    auto ImageData = CoDAssets::GamePackageCache->ExtractPackageObject(ImagePackageName + ".pak", ImageStreamInfo.ImageOffset, (uint64_t)(ImageStreamInfo.ImageEndOffset - ImageStreamInfo.ImageOffset), ResultSize);

    return GameReaders::TranslatePakImage(std::move(ImageData), (uint32_t)ResultSize, ImageInfo.ImageFormat, ImageInfo.MapType, LargestWidth, LargestHeight, Image.ImageUsage);
}

std::string GameModernWarfare2RM::LoadStringEntry(uint64_t Index)
{
    if (ps::state == nullptr)
    {
        return CoDAssets::GameInstance->ReadNullTerminatedString((16 * Index) + CoDAssets::GameOffsetInfos[4] + 8);
    }
    else
    {
        return CoDAssets::GameInstance->ReadNullTerminatedString(ps::state->StringsAddress + Index);
    }
}