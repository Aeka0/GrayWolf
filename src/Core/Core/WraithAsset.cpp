#include "stdafx.h"

#include "WraithAsset.h"

WraithAsset::WraithAsset()
{
    AssetType = WraithAssetType::Unknown;
    AssetName = "WraithAsset";
    AssetStatus = WraithAssetStatus::NotLoaded;
    AssetSize = -1;
    Streamed = false;
}

WraithAsset::~WraithAsset()
{
}