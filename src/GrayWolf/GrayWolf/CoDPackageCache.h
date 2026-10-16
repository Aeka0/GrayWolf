#pragma once

#include <string>
#include <cstdint>
#include <mutex>
#include <vector>
#include <unordered_map>

class CoDFileSystem;

// A structure that represents a package cache object, this is generic and is used in all of the assets
struct PackageCacheObject
{
    uint64_t Offset;
    uint64_t CompressedSize;
    uint64_t UncompressedSize;

    uint32_t PackageFileIndex;
    // A format's own bits (an .xsub of version 23: 1 when the object is framed in blocks, 0 stored)
    uint32_t Flags = 0;
};

// A class that handles reading and indexing game package files for later extraction of data
class CoDPackageCache
{
public:
    CoDPackageCache();
    virtual ~CoDPackageCache();

    bool IsCacheLoading();

    void LoadPackageCacheAsync(const std::string& BasePath);
    void LoadPackageAsync(const std::string& FilePath);

    // Waits for the package cache to load, blocking the current thread
    void WaitForPackageCacheLoad();
    virtual bool Exists(uint64_t CacheID) { return CacheObjects.find(CacheID) != CacheObjects.end(); }
    // Where an object lies (package file, offset in it): lets a caller issue its reads in disk order.
    bool Locate(uint64_t CacheID, uint32_t& Package, uint64_t& Offset) const
    {
        const auto It = CacheObjects.find(CacheID);
        if (It == CacheObjects.end()) return false;
        Package = It->second.PackageFileIndex;
        Offset = It->second.Offset;
        return true;
    }
    // Returns the literal cache object with no /decompression applied (nullptr if not found)
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObjectRaw(uint64_t CacheID, uint32_t& ResultSize) { return nullptr; }
    // Returns a cache object (nullptr if not found)
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObject(uint64_t CacheID, uint32_t& ResultSize) { return ExtractPackageObject(CacheID, -1, ResultSize); }
    // Returns a cache object (nullptr if not found)
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObject(uint64_t CacheID, int32_t Size, uint32_t& ResultSize) { return nullptr; }
    // Returns a cache object (nullptr if not found)
    virtual std::unique_ptr<uint8_t[]> ExtractPackageObject(const std::string& PackageName, uint64_t AssetOffset, uint64_t AssetSize, size_t& ResultSize) { return nullptr; }

    // Hashes a cache object id if needed
    virtual uint64_t HashPackageID(const std::string& Value) { return 0; }

    std::string GetPackagesPath() { return PackageFilesPath; }

    virtual void SetLoadedState();

    virtual CoDFileSystem* GetFileSystem();

protected:

    // Marks the cache as loading, called before the loading thread is spawned so that a
    // waiter can never observe the cache as idle in between
    void BeginLoadingState();

    // Loads the package cache (This should be called from LoadPackageCacheAsync only!)
    virtual void LoadPackageCache(const std::string& BasePath);
    virtual bool LoadPackage(const std::string& FilePath);    

    std::unordered_map<uint64_t, PackageCacheObject> CacheObjects;
    std::vector<std::string> PackageFilePaths;

    // A path of packages, used for games that don't require a cache
    std::string PackageFilesPath;
    
    std::unique_ptr<CoDFileSystem> FileSystem;

    std::string SubDirectory;
private:

    std::mutex CacheMutex;

    bool HasLoaded;
    bool CacheLoading;
};