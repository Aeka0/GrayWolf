#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <functional>
#include <set>
#include <vector>

#include "CoDAssetType.h"

class GameModernWarfareRM
{
public:
    static std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation);
    static std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model);

    static XMaterial_t ReadXMaterial(uint64_t MaterialPointer);

    // A name-only material header (no image: another zone defines it, a map's techsets_<map> zone
    // or a common one) takes its texture slots from there: (semantic, image name) pairs, read by
    // the disk exporter. Unset, the header has no texture.
    static thread_local std::function<bool(const std::string& MaterialName, std::vector<std::pair<uint32_t, std::string>>& Images)> ImagesElsewhere;

    // The sounds one alias names (loaded, streamed, primed), each loaded sound once across calls
    static void AddAliasSounds(const MWRSoundAlias& Alias, const std::string& SoundName, std::set<uint64_t>& Unique, std::vector<CoDAsset_t*>& Out);
};