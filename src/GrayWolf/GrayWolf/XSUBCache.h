#pragma once
#include "CoDAssets.h"
#include <shared_mutex>

class XSUBCache : public CoDPackageCache
{
private:
    std::shared_mutex ReadMutex;
    // Each package's KAPI version (16 Cold War; 23 Modern Warfare III, Black Ops 6 and 7)
    std::vector<uint16_t> PackageVersions;
public:
    XSUBCache();
    virtual ~XSUBCache();

    virtual void LoadPackageCache(const std::string& BasePath);
    virtual bool LoadPackage(const std::string& FilePath);
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObjectRaw(uint64_t CacheID, uint32_t& ResultSize);
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObject(uint64_t CacheID, uint32_t& ResultSize);
    // the sized form the IW live readers call (Modern Warfare III's): an object's size is its own here
    std::unique_ptr<uint8_t[]> ExtractPackageObject(uint64_t CacheID, int32_t Size, uint32_t& ResultSize) override
    {
        return ExtractPackageObject(CacheID, ResultSize);
    }
};