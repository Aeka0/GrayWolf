#include "stdafx.h"

#include "GameModernWarfare4.h"

#include "CoDAssets.h"
#include "ModernIwReaders.h"
#include "Games/Common/MeshDecode.h"
#include "GameReaders.h"
#include "CoDRawImageTranslator.h"
#include "DBGameFiles.h"
#include "FileSystems.h"
#include "MemoryReader.h"
#include "SettingsManager.h"
#include "HalfFloats.h"

#include <filesystem>
#include <format>
#include <fstream>

struct MW4XAssetPoolData
{
    uint64_t PoolPtr;

    uint64_t PoolFreeHeadPtr;

    uint32_t PoolSize;

    uint32_t AssetSize;
};

struct MW4SoundBankInfo
{
    uint64_t BankNamePointer;
    uint64_t StreamKey;
    uint8_t Padding[0x14];
    uint64_t BankFilePointer;
    uint64_t UnkPointer;
    uint32_t BankFileSize;
    uint32_t Unk;
};

struct MW4SoundAlias
{
    uint64_t NamePtr;
    uint32_t NameHash;
    uint32_t Padding;
    uint64_t EntriesPtr;
    uint64_t EntriesCount;
};

struct MW4SoundAliasEntry
{
    uint64_t FilePtr;
    uint32_t FileHash;
    uint32_t Unk;
    uint64_t NamingInfoPtr;
    uint64_t UnkPtr2;
};

// Since Season 5 the Name/Secondary pointers are their own structure, pointed to from the sound alias entry.
struct MW4SoundAliasNamingInfo
{
    uint64_t NamePtr;
    uint64_t SecondaryPtr;
};

uint32_t MW4HashSoundString(const std::string& Value)
{
    uint32_t Result = 5381;

    for (auto& Character : Value)
        Result = (uint32_t)(tolower(Character) + (Result << 6) + (Result << 16)) - Result;

    return Result;
}

static_assert(sizeof(MW4XAssetPoolData) == 0x18, "Invalid Pool Data Size (Expected 0x18)");

bool GameModernWarfare4::LoadAssets()
{
    bool NeedsAnims     = (SettingsManager::GetSetting("showxanim", "true") == "true");
    bool NeedsModels    = (SettingsManager::GetSetting("showxmodel", "true") == "true");
    bool NeedsImages    = (SettingsManager::GetSetting("showximage", "false") == "true");
    bool NeedsSounds    = (SettingsManager::GetSetting("showxsounds", "false") == "true");
    bool NeedsRawFiles  = (SettingsManager::GetSetting("showxrawfiles", "false") == "true");
    bool NeedsMaterials = (SettingsManager::GetSetting("showxmtl", "false") == "true");

    if (NeedsModels)
    {
        auto Pool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 9 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(Pool.Root, CoDAssets::ParasyteRequest, [](ps::XAsset64& Asset)
        {
            auto ModelResult = CoDAssets::GameInstance->Read<MW4XModel>(Asset.Header);
            auto ModelName = FileSystems::GetFileName(CoDAssets::GameInstance->ReadNullTerminatedString(ModelResult.NamePtr));
            auto LoadedModel = new CoDModel_t();
            LoadedModel->AssetName = ModelName;
            LoadedModel->AssetPointer = Asset.Header;
            // Some models have thousands of bones with no info (no parent list): no bones
            if ((ModelResult.NumBones + ModelResult.UnkBoneCount) > 1 && ModelResult.ParentListPtr == 0)
                LoadedModel->BoneCount = 0;
            else
                LoadedModel->BoneCount = ModelResult.NumBones + ModelResult.UnkBoneCount;
            LoadedModel->LodCount = ModelResult.NumLods;
            LoadedModel->AssetStatus = Asset.Temp == 1 ? WraithAssetStatus::Placeholder : WraithAssetStatus::Loaded;
            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedModel);
        });
    }

    if (NeedsImages)
    {
        auto Pool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 19 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(Pool.Root, CoDAssets::ParasyteRequest, [](ps::XAsset64& Asset)
        {
            auto ImageResult = CoDAssets::GameInstance->Read<MW4GfxImage>(Asset.Header);
            uint64_t ImagePointer = *(uint64_t*)&ImageResult.Padding5[8];
            auto ImageName = FileSystems::GetFileName(CoDAssets::GameInstance->ReadNullTerminatedString(ImageResult.NamePtr));
            if (ImagePointer == (uint64_t)-1)
                return;
            auto LoadedImage = new CoDImage_t();
            LoadedImage->AssetName = ImageName;
            LoadedImage->AssetPointer = Asset.Header;
            LoadedImage->Width = ImageResult.LoadedMipWidth;
            LoadedImage->Height = ImageResult.LoadedMipHeight;
            LoadedImage->Format = ImageResult.ImageFormat;
            LoadedImage->AssetStatus = WraithAssetStatus::Loaded;
            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedImage);
        });
    }

    if (NeedsAnims)
    {
        auto Pool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 7 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(Pool.Root, CoDAssets::ParasyteRequest, [](ps::XAsset64& Asset)
        {
            auto AnimResult = CoDAssets::GameInstance->Read<MW4XAnim>(Asset.Header);
            auto AnimName = CoDAssets::GameInstance->ReadNullTerminatedString(AnimResult.NamePtr);

            auto LoadedAnim = new CoDAnim_t();
            LoadedAnim->AssetName = AnimName;
            LoadedAnim->AssetPointer = Asset.Header;
            LoadedAnim->Framerate = AnimResult.Framerate;
            LoadedAnim->FrameCount = AnimResult.NumFrames;
            LoadedAnim->AssetStatus = Asset.Temp == 1 ? WraithAssetStatus::Placeholder : WraithAssetStatus::Loaded;
            LoadedAnim->BoneCount = AnimResult.TotalBoneCount;
            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedAnim);
        });
    }

    if (NeedsMaterials)
    {
        auto Pool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 11 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(Pool.Root, CoDAssets::ParasyteRequest, [](ps::XAsset64& Asset)
        {
            auto MatResult = CoDAssets::GameInstance->Read<MW4XMaterial>(Asset.Header);
            auto MaterialName = CoDAssets::GameInstance->ReadNullTerminatedString(MatResult.NamePtr);

            auto LoadedMaterial = new CoDMaterial_t();
            LoadedMaterial->AssetName = FileSystems::GetFileName(MaterialName);
            LoadedMaterial->AssetPointer = Asset.Header;
            LoadedMaterial->ImageCount = MatResult.ImageCount;
            LoadedMaterial->AssetStatus = WraithAssetStatus::Loaded;

            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedMaterial);
        });
    }

    // MW stores the entire SABL in fast files, so it is parsed in memory; SABS files load as usual.
    if (NeedsSounds)
    {
        auto Pool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 21 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(Pool.Root, CoDAssets::ParasyteRequest, [](ps::XAsset64& Asset)
        {
            auto SoundResult = CoDAssets::GameInstance->Read<MW4SoundBank>(Asset.Header);
            auto SoundBankInfo = CoDAssets::GameInstance->Read<MW4SoundBankInfo>(SoundResult.SoundBankPtr);

            if (SoundBankInfo.BankFilePointer > 0)
            {
                std::map<uint32_t, std::string> SABFileNames;

                auto Header = CoDAssets::GameInstance->Read<SABFileHeader>(SoundBankInfo.BankFilePointer);

                // Verify magic first, same for all SAB files
                // The magic is ('2UX#')
                if (Header.Magic != 0x23585532)
                {
                    return;
                }

                auto SkipBlankAudio = SettingsManager::GetSetting("skipblankaudio", "false") == "true";

                auto NamesOffset = CoDAssets::GameInstance->Read<uint64_t>(SoundBankInfo.BankFilePointer + 0x250);

                for (size_t i = 0; i < Header.EntriesCount; i++)
                {
                    auto Name = CoDAssets::GameInstance->ReadNullTerminatedString(SoundBankInfo.BankFilePointer + NamesOffset + i * 128);
                    SABFileNames[MW4HashSoundString(Name)] = Name;
                }

                for (uint32_t i = 0; i < Header.EntriesCount; i++)
                {
                    auto Entry = CoDAssets::GameInstance->Read<SABv4Entry>(SoundBankInfo.BankFilePointer + Header.EntryTableOffset + i * sizeof(SABv4Entry));

                    std::string EntryName = "";
                    if (SABFileNames.size() > 0)
                    {
                        EntryName = SABFileNames[Entry.Key];
                    }
                    else
                    {
                        EntryName = std::format("_{:x}", Entry.Key);
                    }

                    auto LoadedSound = new CoDSound_t();
                    LoadedSound->AssetName = FileSystems::GetFileNamePurgeExtensions(EntryName);
                    LoadedSound->FullPath = FileSystems::GetDirectoryName(EntryName);

                    LoadedSound->FrameRate = Entry.FrameRate;
                    LoadedSound->FrameCount = Entry.FrameCount;
                    LoadedSound->ChannelsCount = Entry.ChannelCount;
                    // Skip the seek table, not required
                    LoadedSound->AssetPointer = SoundBankInfo.BankFilePointer + (Entry.Offset + Entry.SeekTableLength);
                    LoadedSound->AssetSize = Entry.Size;
                    LoadedSound->AssetStatus = WraithAssetStatus::Loaded;
                    LoadedSound->IsFileEntry = false;
                    LoadedSound->Length = (uint32_t)(1000.0f * (float)(LoadedSound->FrameCount / (float)(LoadedSound->FrameRate)));
                    // All Modern Warfare (v10) entries are FLAC's with no header
                    LoadedSound->DataType = SoundDataTypes::Opus_Interleaved;

                    if (SkipBlankAudio && LoadedSound->AssetSize <= 0)
                    {
                        delete LoadedSound;
                        continue;
                    }

                    CoDAssets::GameAssets->LoadedAssets.push_back(LoadedSound);
                }
            }
        });
    }

    if (NeedsRawFiles)
    {
        auto Pool = CoDAssets::GameInstance->Read<ps::XAssetPool64>(ps::state->PoolsAddress + 22 * sizeof(ps::XAssetPool64));
        ps::PoolParser64(Pool.Root, CoDAssets::ParasyteRequest, [](ps::XAsset64& Asset)
        {
            auto SoundResult = CoDAssets::GameInstance->Read<MW4SoundBank>(Asset.Header);
            auto RawfileName = CoDAssets::GameInstance->ReadNullTerminatedString(SoundResult.NamePtr) + ".sabs";

            // Actually the streamer info pool
            auto Info = CoDAssets::GameInstance->Read<MW4SoundBankInfo>(SoundResult.SoundBankPtr);

            auto LoadedRawfile = new CoDRawFile_t();
            LoadedRawfile->AssetName = FileSystems::GetFileName(RawfileName);
            LoadedRawfile->RawFilePath = FileSystems::GetDirectoryName(RawfileName);
            LoadedRawfile->AssetPointer = Asset.Header;
            LoadedRawfile->AssetSize = Info.BankFileSize;
            LoadedRawfile->RawDataPointer = Info.BankFilePointer;
            LoadedRawfile->AssetStatus = WraithAssetStatus::Loaded;

            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedRawfile);
        });
    }

    return true;
}

std::unique_ptr<XAnim_t> GameModernWarfare4::ReadXAnim(const CoDAnim_t* Animation)
{
    if (CoDAssets::GameInstance->IsRunning())
    {
        auto Anim = std::make_unique<XAnim_t>();

        auto AnimData = CoDAssets::GameInstance->Read<MW4XAnim>(Animation->AssetPointer);

        Anim->AnimationName = Animation->AssetName;
        Anim->FrameCount = AnimData.NumFrames;
        Anim->FrameRate = AnimData.Framerate;

        // No point, breaks it in SETools, wait for Cast to implement full Additive support
        if (AnimData.AssetType == 0x6)
        {
            Anim->AdditiveAnimation = true;
        }
        Anim->LoopingAnimation = (AnimData.Flags & 1);

        auto AnimDeltaData = CoDAssets::GameInstance->Read<MW4XAnimDeltaParts>(AnimData.DeltaPartsPtr);

        Anim->BoneIDsPtr = AnimData.BoneIDsPtr;
        Anim->DataBytesPtr = AnimData.DataBytePtr;
        Anim->DataShortsPtr = AnimData.DataShortPtr;
        Anim->DataIntsPtr = AnimData.DataIntPtr;
        Anim->RandomDataBytesPtr = AnimData.RandomDataBytePtr;
        Anim->RandomDataShortsPtr = AnimData.RandomDataShortPtr;
        Anim->RandomDataIntsPtr = AnimData.RandomDataIntPtr;
        Anim->LongIndiciesPtr = AnimData.LongIndiciesPtr;
        Anim->NotificationsPtr = AnimData.NotificationsPtr;

        Anim->BoneIndexSize = 4;

        Anim->NoneRotatedBoneCount = AnimData.NoneRotatedBoneCount;
        Anim->TwoDRotatedBoneCount = AnimData.TwoDRotatedBoneCount;
        Anim->NormalRotatedBoneCount = AnimData.NormalRotatedBoneCount;
        Anim->TwoDStaticRotatedBoneCount = AnimData.TwoDStaticRotatedBoneCount;
        Anim->NormalStaticRotatedBoneCount = AnimData.NormalStaticRotatedBoneCount;
        Anim->NormalTranslatedBoneCount = AnimData.NormalTranslatedBoneCount;
        Anim->PreciseTranslatedBoneCount = AnimData.PreciseTranslatedBoneCount;
        Anim->StaticTranslatedBoneCount = AnimData.StaticTranslatedBoneCount;
        Anim->NoneTranslatedBoneCount = AnimData.NoneTranslatedBoneCount;
        Anim->TotalBoneCount = AnimData.TotalBoneCount;
        Anim->NotificationCount = AnimData.NotificationCount;

        Anim->DeltaTranslationPtr = AnimDeltaData.DeltaTranslationsPtr;
        Anim->Delta2DRotationsPtr = AnimDeltaData.Delta2DRotationsPtr;
        Anim->Delta3DRotationsPtr = AnimDeltaData.Delta3DRotationsPtr;

        Anim->RotationType = AnimationKeyTypes::DivideBySize;
        Anim->TranslationType = AnimationKeyTypes::MinSizeTable;

        Anim->SupportsInlineIndicies = true;

        return Anim;
    }
    return nullptr;
}

std::unique_ptr<XModel_t> GameModernWarfare4::ReadXModel(const CoDModel_t* Model)
{
    if (CoDAssets::GameInstance->IsRunning())
    {
        auto ModelData = CoDAssets::GameInstance->Read<MW4XModel>(Model->AssetPointer);

        auto ModelAsset = std::make_unique<XModel_t>(ModelData.NumLods);

        ModelAsset->ModelName = Model->AssetName;
        // Some models have thousands of bones with no info (no parent list): no bones
        if ((ModelData.NumBones + ModelData.UnkBoneCount) > 1 && ModelData.ParentListPtr == 0)
        {
            ModelAsset->BoneCount = 0;
            ModelAsset->RootBoneCount = 0;
        }
        else
        {
            ModelAsset->BoneCount = ModelData.NumBones + ModelData.UnkBoneCount;
            ModelAsset->RootBoneCount = ModelData.NumRootBones;
        }
        ModelAsset->BoneRotationData = BoneDataTypes::DivideBySize;

        ModelAsset->IsModelStreamed = true;

        ModelAsset->BoneIDsPtr = ModelData.BoneIDsPtr;
        ModelAsset->BoneIndexSize = 4;

        ModelAsset->BoneParentsPtr = ModelData.ParentListPtr;
        ModelAsset->BoneParentSize = 1;

        ModelAsset->RotationsPtr = ModelData.RotationsPtr;
        ModelAsset->TranslationsPtr = ModelData.TranslationsPtr;

        ModelAsset->BaseMatriciesPtr = ModelData.BaseMatriciesPtr;

        for (uint32_t i = 0; i < ModelData.NumLods; i++)
        {
            ModelAsset->ModelLods.emplace_back(ModelData.ModelLods[i].NumSurfs);
            auto& LodReference = ModelAsset->ModelLods[i];

            LodReference.LodDistance = ModelData.ModelLods[i].LodDistance;

            LodReference.LODStreamInfoPtr = ModelData.ModelLods[i].MeshPtr;

            auto XSurfacePtr = ModelData.ModelLods[i].SurfsPtr;

            for (uint32_t s = 0; s < ModelData.ModelLods[i].NumSurfs; s++)
            {
                LodReference.Submeshes.emplace_back();
                auto& SubmeshReference = LodReference.Submeshes[s];

                auto SurfaceInfo = CoDAssets::GameInstance->Read<MW4XModelSurface>(XSurfacePtr);

                SubmeshReference.RigidWeightsPtr  = SurfaceInfo.RigidWeightsPtr;
                SubmeshReference.VertListcount    = SurfaceInfo.VertListCount;
                SubmeshReference.VertexCount      = SurfaceInfo.VertexCount;
                SubmeshReference.FaceCount        = SurfaceInfo.FacesCount;
                SubmeshReference.VertexPtr        = SurfaceInfo.Offsets[0];
                SubmeshReference.VertexUVsPtr     = SurfaceInfo.Offsets[2];
                SubmeshReference.VertexNormalsPtr = SurfaceInfo.Offsets[3];
                SubmeshReference.FacesPtr         = SurfaceInfo.Offsets[4];
                SubmeshReference.VertexColorPtr   = SurfaceInfo.Offsets[6];
                SubmeshReference.Scale            = fmaxf(fmaxf(SurfaceInfo.Min, SurfaceInfo.Scale), SurfaceInfo.Max);
                SubmeshReference.XOffset          = SurfaceInfo.XOffset;
                SubmeshReference.YOffset          = SurfaceInfo.YOffset;
                SubmeshReference.ZOffset          = SurfaceInfo.ZOffset;

                SubmeshReference.WeightCounts[0] = SurfaceInfo.WeightCounts[0];
                SubmeshReference.WeightCounts[1] = SurfaceInfo.WeightCounts[1];
                SubmeshReference.WeightCounts[2] = SurfaceInfo.WeightCounts[2];
                SubmeshReference.WeightCounts[3] = SurfaceInfo.WeightCounts[3];
                SubmeshReference.WeightCounts[4] = SurfaceInfo.WeightCounts[4];
                SubmeshReference.WeightCounts[5] = SurfaceInfo.WeightCounts[5];
                SubmeshReference.WeightCounts[6] = SurfaceInfo.WeightCounts[6];
                SubmeshReference.WeightCounts[7] = SurfaceInfo.WeightCounts[7];
                SubmeshReference.WeightsPtr = SurfaceInfo.WeightsPtr;

                auto MaterialHandle = CoDAssets::GameInstance->Read<uint64_t>(ModelData.MaterialHandlesPtr);
                LodReference.Materials.emplace_back(ReadXMaterial(MaterialHandle));

                XSurfacePtr += sizeof(MW4XModelSurface);
                ModelData.MaterialHandlesPtr += sizeof(uint64_t);
            }
        }

        return ModelAsset;
    }
    return nullptr;
}

std::unique_ptr<XImageDDS> GameModernWarfare4::ReadXImage(const CoDImage_t* Image)
{
    return LoadXImage(XImage_t(ImageUsageType::DiffuseMap, 0, Image->AssetPointer, Image->AssetName));
}

void GameModernWarfare4::TranslateRawfile(const CoDRawFile_t * Rawfile, const std::string & ExportPath)
{
    std::error_code Ec;
    std::string ExportFolder = ExportPath;

    if (SettingsManager::GetSetting("keeprawpath", "true") == "true" && !Rawfile->RawFilePath.empty())
    {
        ExportFolder = (std::filesystem::path(ExportFolder) / Rawfile->RawFilePath).string();
    }

    std::filesystem::create_directories(ExportFolder, Ec);

    auto Bank = CoDAssets::GameInstance->Read<MW4SoundBank>(Rawfile->AssetPointer);


    std::ofstream AliasWriter((std::filesystem::path(ExportFolder) / (Rawfile->AssetName + ".csv")).string());
    if (AliasWriter)
    {
        AliasWriter << "Name,Secondary,File\n";
        AliasWriter << "# Note: Secondary is triggered by this alias, so a chain of secondaries are played at the same time\n";
        AliasWriter << "# Note: There are other settings that cannot be pulled, so these sounds may not sound the same as in-game without editing\n";

        for (uint64_t j = 0; j < Bank.AliasCount; j++)
        {
            auto Alias = CoDAssets::GameInstance->Read<MW4SoundAlias>(Bank.AliasesPtr + j * sizeof(MW4SoundAlias));

            for (uint64_t k = 0; k < Alias.EntriesCount; k++)
            {
                auto Entry = CoDAssets::GameInstance->Read<MW4SoundAliasEntry>(Alias.EntriesPtr + k * sizeof(MW4SoundAliasEntry));

                auto NamingInfo = CoDAssets::GameInstance->Read<MW4SoundAliasNamingInfo>(Entry.NamingInfoPtr);
                auto Name      = CoDAssets::GameInstance->ReadNullTerminatedString(NamingInfo.NamePtr);
                auto Secondary = CoDAssets::GameInstance->ReadNullTerminatedString(NamingInfo.SecondaryPtr);
                auto FileName  = CoDAssets::GameInstance->ReadNullTerminatedString(Entry.FilePtr);

                AliasWriter << std::format("{},{},{}\n", Name.c_str(), Secondary.c_str(), FileName.c_str());
            }
        }
    }

    // Actually the streamer info pool
    auto Info = CoDAssets::GameInstance->Read<MW4SoundBankInfo>(Bank.SoundBankPtr);

    // Always stream: the data in memory is unreliable
    uint32_t ResultSize = 0;
    std::unique_ptr<uint8_t[]> Data = CoDAssets::GamePackageCache->ExtractPackageObject(Info.StreamKey, Info.BankFileSize, ResultSize);

    if (Data != nullptr)
    {
        auto Writer = BinaryWriter();
        if (Writer.Create((std::filesystem::path(ExportFolder) / Rawfile->AssetName).string()))
        {
            Writer.Write(Data.get(), (uint32_t)Info.BankFileSize);
        }
    }
}

thread_local GameReaders::ImagesElsewhereFn GameModernWarfare4::ImagesElsewhere;

const XMaterial_t GameModernWarfare4::ReadXMaterial(uint64_t MaterialPointer)
{
    auto MaterialData = CoDAssets::GameInstance->Read<MW4XMaterial>(MaterialPointer);

    XMaterial_t Result(MaterialData.ImageCount);
    const auto RawName = CoDAssets::GameInstance->ReadNullTerminatedString(MaterialData.NamePtr);
    Result.MaterialName = FileSystems::GetFileNameWithoutExtension(RawName);

    for (uint32_t m = 0; m < MaterialData.ImageCount; m++)
    {
        auto ImageInfo = CoDAssets::GameInstance->Read<MW4XMaterialImage>(MaterialData.ImageTablePtr);
        // Image name pointer: end of image - 8
        auto ImageName = CoDAssets::GameInstance->ReadNullTerminatedString(CoDAssets::GameInstance->Read<uint64_t>(ImageInfo.ImagePtr));

        auto DefaultUsage = ImageUsageType::Unknown;
        switch (ImageInfo.Type)
        {
        case 0:
            DefaultUsage = ImageUsageType::DiffuseMap;
            break;
        }

        Result.Images.emplace_back(DefaultUsage, ImageInfo.Type, ImageInfo.ImagePtr, ImageName);

        MaterialData.ImageTablePtr += sizeof(MW4XMaterialImage);
    }

    if (MaterialData.ImageCount == 0 && !RawName.empty() && RawName[0] == ',')
        GameReaders::AddImagesElsewhere(Result, ImagesElsewhere, RawName.substr(1),
                                        [](uint32_t Semantic) { return Semantic == 0 ? ImageUsageType::DiffuseMap : ImageUsageType::Unknown; });

    return Result;
}

std::unique_ptr<XImageDDS> GameModernWarfare4::LoadXImage(const XImage_t& Image)
{
    std::unique_ptr<uint8_t[]> ImageData = nullptr;
    // Both loaded and streamed images are read
    uint32_t ResultSize = 0;

    auto ImageInfo = CoDAssets::GameInstance->Read<MW4GfxImage>(Image.ImagePtr);
    uint64_t ImagePointer = *(uint64_t*)&ImageInfo.Padding5[8];
    uint32_t LoadedImageSize = *(uint32_t*)&ImageInfo.Padding2[7];

    uint32_t Width = 0;
    uint32_t Height = 0;

    if (ImagePointer != 0)
    {
        ImageData = std::make_unique<uint8_t[]>(LoadedImageSize);

        if (CoDAssets::GameInstance->Read(ImageData.get(), ImagePointer, LoadedImageSize) != LoadedImageSize)
            return nullptr;

        Width = ImageInfo.LoadedMipWidth;
        Height = ImageInfo.LoadedMipHeight;
        ResultSize = LoadedImageSize;
    }
    else if (CoDAssets::GamePackageCache != nullptr)
    {
        // Fallback: the best mip present in the local files, in case the CDN is not viable
        size_t Fallback = 0;
        size_t HighestIndex = 4;

        for (size_t i = 0; i < 4; i++)
        {
            if (ImageInfo.Mips.Levels[i].HashID != 0)
            {
                if (CoDAssets::GamePackageCache->Exists(ImageInfo.Mips.Levels[i].HashID))
                {
                    Fallback = i;
                }

                HighestIndex = i;
            }
        }

        // Highest mip not in the files: try the on-demand cache, then the CDN
        // (no on-demand cache when the zone is read from disk)
        if (HighestIndex != Fallback && CoDAssets::OnDemandCache != nullptr)
        {
            ImageData = CoDAssets::OnDemandCache->ExtractPackageObject(ImageInfo.Mips.Levels[HighestIndex].HashID, ImageInfo.Mips.GetImageSize(HighestIndex), ResultSize);

            if (ImageData == nullptr && CoDAssets::CDNDownloader != nullptr)
            {
                size_t CDNSize = 0;
                ImageData = CoDAssets::CDNDownloader->ExtractCDNObject(ImageInfo.Mips.Levels[HighestIndex].HashID, ImageInfo.Mips.GetImageSize(HighestIndex), CDNSize);
                ResultSize = (uint32_t)CDNSize;
            }
        }

        // No CDN version available: fall back to the files
        if (ImageData == nullptr)
        {
            ImageData = CoDAssets::GamePackageCache->ExtractPackageObject(ImageInfo.Mips.Levels[Fallback].HashID, ImageInfo.Mips.GetImageSize(Fallback), ResultSize);
            HighestIndex = Fallback;
        }

        Width = ImageInfo.Mips.Levels[HighestIndex].Width;
        Height = ImageInfo.Mips.Levels[HighestIndex].Height;
    }

    if (ImageData == nullptr)
        return nullptr;

    switch (ImageInfo.ImageFormat)
    {
    case 0:  ImageInfo.ImageFormat = 71; break;
    case 1:  ImageInfo.ImageFormat = 61; break;
    case 2:  ImageInfo.ImageFormat = 65; break;
    case 3:  ImageInfo.ImageFormat = 65; break;
    case 4:  ImageInfo.ImageFormat = 49; break;
    case 5:  ImageInfo.ImageFormat = 49; break;
    case 6:  ImageInfo.ImageFormat = 28; break;
    case 7:  ImageInfo.ImageFormat = 28; break;
    case 8:  ImageInfo.ImageFormat = 71; break;
    case 9:  ImageInfo.ImageFormat = 71; break;
    case 10: ImageInfo.ImageFormat = 63; break;
    case 11: ImageInfo.ImageFormat = 51; break;
    case 12: ImageInfo.ImageFormat = 56; break;
    case 13: ImageInfo.ImageFormat = 35; break;
    case 14: ImageInfo.ImageFormat = 11; break;
    case 15: ImageInfo.ImageFormat = 58; break;
    case 16: ImageInfo.ImageFormat = 54; break;
    case 17: ImageInfo.ImageFormat = 34; break;
    case 18: ImageInfo.ImageFormat = 10; break;
    case 19: ImageInfo.ImageFormat = 41; break;
    case 20: ImageInfo.ImageFormat = 16; break;
    case 21: ImageInfo.ImageFormat = 2;  break;
    case 22: ImageInfo.ImageFormat = 40; break;
    case 23: ImageInfo.ImageFormat = 20; break;
    case 24: ImageInfo.ImageFormat = 62; break;
    case 25: ImageInfo.ImageFormat = 57; break;
    case 26: ImageInfo.ImageFormat = 42; break;
    case 27: ImageInfo.ImageFormat = 17; break;
    case 28: ImageInfo.ImageFormat = 3;  break;
    case 29: ImageInfo.ImageFormat = 25; break;
    case 30: ImageInfo.ImageFormat = 85; break;
    case 31: ImageInfo.ImageFormat = 85; break;
    case 32: ImageInfo.ImageFormat = 24; break;
    case 33: ImageInfo.ImageFormat = 67; break;
    case 34: ImageInfo.ImageFormat = 67; break;
    case 35: ImageInfo.ImageFormat = 71; break;
    case 36: ImageInfo.ImageFormat = 71; break;
    case 37: ImageInfo.ImageFormat = 74; break;
    case 38: ImageInfo.ImageFormat = 74; break;
    case 39: ImageInfo.ImageFormat = 77; break;
    case 40: ImageInfo.ImageFormat = 77; break;
    case 41: ImageInfo.ImageFormat = 80; break;
    case 42: ImageInfo.ImageFormat = 84; break;
    case 43: ImageInfo.ImageFormat = 84; break;
    case 44: ImageInfo.ImageFormat = 95; break;
    case 45: ImageInfo.ImageFormat = 96; break;
    case 46: ImageInfo.ImageFormat = 98; break;
    case 47: ImageInfo.ImageFormat = 98; break;
    default: ImageInfo.ImageFormat = 71; break;
    }

    auto Result = CoDRawImageTranslator::TranslateBC(ImageData, ResultSize, Width, Height, ImageInfo.ImageFormat);
    return Result;
}

void GameModernWarfare4::LoadXModel(const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel)
{
    bool ExportColors = (SettingsManager::GetSetting("exportvtxcolor", "true") == "true");
    auto MeshInfo = CoDAssets::GameInstance->Read<MW4XModelMesh>(ModelLOD.LODStreamInfoPtr);
    auto BufferInfo = CoDAssets::GameInstance->Read<MW4XModelMeshBufferInfo>(MeshInfo.MeshBufferPointer);

    // Even a streamed mesh can be in memory if loaded
    const auto Mesh = BufferInfo.BufferPtr != 0 ? MeshDecode::FromZone(BufferInfo.BufferPtr, BufferInfo.BufferSize)
                                                : MeshDecode::FromPackage(MeshInfo.LODStreamKey, BufferInfo.BufferSize);

    // 21-bit positions, packed quaternion normals, half UVs, u16 faces; rigid runs of 8 bytes, the
    // skinned weights in the zone
    constexpr MeshDecode::LodFormat Format{ { MeshDecode::PositionCodec::Packed21, MeshDecode::NormalCodec::PackedQuat, MeshDecode::UvCodec::Half2,
                                              MeshDecode::FaceCodec::U16 }, { 8, 2 }, false };
    if (Mesh.Data != nullptr)
        MeshDecode::DecodeLod(Mesh.View(), ModelLOD, Format, ExportColors, *ResultModel);
}

std::string GameModernWarfare4::LoadStringEntry(uint64_t Index)
{
    return CoDAssets::GameInstance->ReadNullTerminatedString(ps::state->StringsAddress + Index);
}
void GameModernWarfare4::PerformInitialSetup()
{
    std::error_code Ec;
    auto OurPath = (std::filesystem::path(FileSystems::GetApplicationPath()) / "oo2core_6_win64.dll").string();

    if (!std::filesystem::is_regular_file(OurPath, Ec))
        std::filesystem::copy_file((std::filesystem::path(ps::state->GameDirectory) / "oo2core_7_win64.dll").string(), OurPath, std::filesystem::copy_options::overwrite_existing, Ec);
}
