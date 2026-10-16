#include "stdafx.h"

#include "GameGhosts.h"

#include "GameReaders.h"
#include "PAKSupport.h"

void GameGhosts::AddAliasSounds(const GhostsSoundAlias& Alias, const std::string& SoundName, std::set<uint64_t>& Unique, std::vector<CoDAsset_t*>& Out)
{
    for (uint32_t j = 0; j < Alias.EntryCount; j++)
    {
        auto SoundAliasEntry = CoDAssets::GameInstance->Read<GhostsSoundAliasEntry>(Alias.EntriesPtr + (j * sizeof(GhostsSoundAliasEntry)));
        auto SoundFileSpec = CoDAssets::GameInstance->Read<AWSoundAliasFileSpec>(SoundAliasEntry.FileSpecPtr);
        if (SoundFileSpec.Type == 1)
        {
            auto LoadedSoundPtr = CoDAssets::GameInstance->Read<uint64_t>(SoundAliasEntry.FileSpecPtr + 8);
            if (Unique.insert(LoadedSoundPtr).second == false)
                continue;
            auto LoadedSoundInfo = CoDAssets::GameInstance->Read<GhostsLoadedSound>(LoadedSoundPtr);
            auto LoadedSoundName = CoDAssets::GameInstance->ReadNullTerminatedString(LoadedSoundInfo.NamePtr);
            Out.push_back(GameReaders::LoadedSound(LoadedSoundInfo, LoadedSoundName, SoundDataTypes::WAV_NeedsHeader));
        }
        else if (SoundFileSpec.Type == 2)
        {
            if (Unique.insert(SoundAliasEntry.FileSpecPtr).second == false)
                continue;
            auto StreamedSoundInfo = CoDAssets::GameInstance->Read<GhostsStreamedSound>(SoundAliasEntry.FileSpecPtr + 8);
            Out.push_back(GameReaders::StreamedSound(StreamedSoundInfo, SoundName, j));
        }
        else if (SoundFileSpec.Type == 3)
        {
            auto PrimedAudioInfo = CoDAssets::GameInstance->Read<GhostsPrimedSound>(SoundAliasEntry.FileSpecPtr + 8);
            if (Unique.insert(PrimedAudioInfo.LoadedSoundPtr).second == false)
                continue;
            auto LoadedSoundInfo = CoDAssets::GameInstance->Read<GhostsLoadedSound>(PrimedAudioInfo.LoadedSoundPtr);
            auto LoadedSoundName = CoDAssets::GameInstance->ReadNullTerminatedString(LoadedSoundInfo.NamePtr);
            Out.push_back(GameReaders::PrimedSound(LoadedSoundInfo, LoadedSoundName, PrimedAudioInfo, SoundDataTypes::FLAC_WithHeader));
        }
    }
}

std::unique_ptr<XAnim_t> GameGhosts::ReadXAnim(const CoDAnim_t* Animation)
{
    return GameReaders::ReadAnim<GhostsXAnim, GhostsXAnimDeltaParts>(Animation, 4, { "viewmodel_" });
}

std::unique_ptr<XModel_t> GameGhosts::ReadXModel(const CoDModel_t* Model)
{
    return GameReaders::ReadModel<GhostsXModel, GhostsXModelSurface, uint64_t>(Model, 4, ReadXMaterial);
}

const XMaterial_t GameGhosts::ReadXMaterial(uint64_t MaterialPointer)
{
    auto MaterialData = CoDAssets::GameInstance->Read<GhostsXMaterial>(MaterialPointer);

    XMaterial_t Result(MaterialData.ImageCount);
    Result.MaterialName = FileSystems::GetFileNameWithoutExtension(CoDAssets::GameInstance->ReadNullTerminatedString(MaterialData.NamePtr));
    // Image name pointer: end of image - 8
    GameReaders::AddMaterialImages<GhostsXMaterialImage>(Result, MaterialData.ImageTablePtr, MaterialData.ImageCount,
                                                         [](uint64_t Image) { return GameReaders::ImageNameAt<uint64_t>(Image, sizeof(GhostsGfxImage) - 8); },
                                                         GameReaders::SemanticUsage);
    return Result;
}

std::unique_ptr<XImageDDS> GameGhosts::DecodePakImage(const GhostsGfxImage& Info, const std::string& PakPath, uint64_t Start, uint64_t End, uint32_t Width, uint32_t Height, ImageUsageType Usage)
{
    uint32_t ResultSize = 0;
    auto ImageData = PAKSupport::GhostsExtractImagePackage(PakPath, Start, End - Start, ResultSize);
    return GameReaders::TranslatePakImage(std::move(ImageData), ResultSize, Info.ImageFormat, Info.MapType, Width, Height, Usage);
}
