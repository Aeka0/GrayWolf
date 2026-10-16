#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <functional>
#include <set>
#include <vector>

#include "CoDAssetType.h"

class GameAdvancedWarfare
{
public:
    static std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation);
    static std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model);

    // One image's pixels (Levels mips from the largest) as a DDS: A2XY normal maps un-swapped,
    // normals patched
    static std::unique_ptr<XImageDDS> TranslateImage(std::unique_ptr<uint8_t[]> Data, uint32_t Size, uint8_t Format, uint8_t MapType,
                                                     uint32_t Width, uint32_t Height, ImageUsageType Usage, uint8_t Levels = 1);
    // Inflates one imagefile<N>.pak stream entry and translates it at the given size (the live
    // reader and the disk exporter find the entry differently)
    static std::unique_ptr<XImageDDS> DecodePakImage(const AWGfxImage& Info, const std::string& PakPath, uint64_t Start, uint64_t End,
                                                     uint32_t Width, uint32_t Height, ImageUsageType Usage);

    // The sounds one alias names (loaded, streamed, primed), each file spec once across calls.
    // FromPackages: a loaded sound's samples from its soundfile<N>.pak copy (a zone on disk holds
    // zeros where the game loads them)
    static void AddAliasSounds(const AWSoundAlias& Alias, const std::string& SoundName, std::set<uint64_t>& Unique, std::vector<CoDAsset_t*>& Out,
                               bool FromPackages = false);

    static const XMaterial_t ReadXMaterial(uint64_t MaterialPointer);
    // A material another zone defines (a name-only header without images, its name without the
    // leading ','): its images as (semantic hash, image name), when the caller knows them. Read
    // by name only: ReadXMaterial gives them the ExportWriters::ImageElsewhere pointer.
    static thread_local std::function<bool(const std::string& Name, std::vector<std::pair<uint32_t, std::string>>& Images)> ImagesElsewhere;
};