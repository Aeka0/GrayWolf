#pragma once

#include <string>
#include <cstdint>
#include <map>
#include <memory>

#include "CoDPackageCache.h"

class IPAKCache : public CoDPackageCache
{
public:
    IPAKCache();
    virtual ~IPAKCache();

    virtual void LoadPackageCache(const std::string& BasePath);
    virtual bool LoadPackage(const std::string& FilePath);
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObject(uint64_t CacheID, uint32_t& ResultSize);

private:
    // Entry.Offset -> Entry.Key, across all loaded packages (multiple packages can share an offset).
    std::multimap<uint64_t, uint64_t> KeyByOffset;
};