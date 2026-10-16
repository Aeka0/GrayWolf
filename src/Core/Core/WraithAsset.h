#pragma once

#include <cstdint>
#include <string>

enum class WraithAssetType
{
    Animation,
    Image,
    Model,
    Sound,
    Effect,
    RawFile,
    Material,
    Custom,
    Unknown,
};

enum class WraithAssetStatus
{
    Loaded,
    Exported,
    NotLoaded,
    Placeholder,
    Processing,
    Error
};

// A class that represents a WraithAsset to be shown in the asset list
class WraithAsset
{
public:
    WraithAsset();
    virtual ~WraithAsset();

    WraithAssetType AssetType;
    std::string AssetName;
    WraithAssetStatus AssetStatus;
    // The size of this asset, -1 = N/A
    int64_t AssetSize;
    bool Streamed;
};