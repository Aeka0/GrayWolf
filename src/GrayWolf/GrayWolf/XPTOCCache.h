#pragma once

#include <string>
#include <cstdint>
#include <memory>

#include "CoDPackageCache.h"

class XPTOCCache : public CoDPackageCache
{
public:
    XPTOCCache();
    virtual ~XPTOCCache();

    virtual void LoadPackageCache(const std::string& BasePath);
    virtual bool LoadPackage(const std::string& FilePath);
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObject(uint64_t CacheID, uint32_t& ResultSize);
};