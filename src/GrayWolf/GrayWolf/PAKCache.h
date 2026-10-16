#pragma once

#include <string>
#include <cstdint>

#include "CoDPackageCache.h"

class PAKCache : public CoDPackageCache
{
public:
    PAKCache();
    virtual ~PAKCache();

    virtual void LoadPackageCache(const std::string& BasePath);
    // Returns a cache object (nullptr if not found)
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObject(const std::string& PackageName, const uint64_t AssetOffset, const uint64_t AssetSize, size_t& ResultSize);
};