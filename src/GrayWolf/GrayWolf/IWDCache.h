#pragma once

#include <string>
#include <cstdint>

#include "CoDPackageCache.h"

class IWDCache : public CoDPackageCache
{
public:
    IWDCache();
    virtual ~IWDCache();

    virtual void LoadPackageCache(const std::string& BasePath);
    virtual bool LoadPackage(const std::string& FilePath);
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObject(uint64_t CacheID, uint32_t& ResultSize);
    virtual uint64_t HashPackageID(const std::string& Value);
};