#include "stdafx.h"

#include "CoDPackageCache.h"

#include "WinFileSystem.h"
#include "CascFileSystem.h"

#include <filesystem>

CoDPackageCache::CoDPackageCache()
{
    HasLoaded = false;
    CacheLoading = false;
}

CoDPackageCache::~CoDPackageCache()
{
    CacheObjects.clear();
    PackageFilePaths.clear();
    PackageFilePaths.shrink_to_fit();
}

bool CoDPackageCache::IsCacheLoading()
{
    std::lock_guard<std::mutex> Lock(CacheMutex);

    return CacheLoading;
}

void CoDPackageCache::LoadPackageCache(const std::string& BasePath)
{
    std::error_code Ec;
    std::lock_guard<std::mutex> Lock(CacheMutex);

#ifdef _DEBUG
    printf("LoadPackageCache(): Begin loading...\n");
#endif

    CacheLoading = true;

    // Open the file system, check for build info, if build info exists
    // we'll use Casc, otherwise use raw directory.
    if (std::filesystem::is_regular_file(BasePath + "\\.build.info", Ec))
    {
        FileSystem = std::make_unique<CascFileSystem>(BasePath);
    }
    else
    {
        FileSystem = std::make_unique<WinFileSystem>(BasePath);
    }

    if (!FileSystem->IsValid())
    {
        // Say so: otherwise every later extraction silently returns nothing (Black Ops 4's assets
        // live in a Battle.net CASC container, not in loose files).
        printf("LoadPackageCache(): could not open '%s' - packaged assets will be unavailable\n",
               BasePath.c_str());

        FileSystem = nullptr;
    }
}

bool CoDPackageCache::LoadPackage(const std::string& FilePath)
{
    // Nothing, default, but ensure status is loading
    CacheLoading = true;
    HasLoaded = false;
    return true;
}

void CoDPackageCache::SetLoadedState()
{
    std::lock_guard<std::mutex> Lock(CacheMutex);

#ifdef _DEBUG
    printf("SetLoadedState(): Cache loaded %d objects.\n", (uint32_t)CacheObjects.size());
#endif

    HasLoaded = true;
    CacheLoading = false;
}

CoDFileSystem* CoDPackageCache::GetFileSystem()
{
    return FileSystem.get();
}

void CoDPackageCache::LoadPackageCacheAsync(const std::string& BasePath)
{
    // Flag the load here rather than on the worker. Setting it once the thread is
    // running leaves a window where WaitForPackageCacheLoad sees an idle cache and
    // returns immediately, so the export runs against an index that has not been built
    // yet, extracts nothing, and still reports every asset as exported.
    BeginLoadingState();

    std::thread LoadThread([BasePath, this]
    {
        this->LoadPackageCache(BasePath);
    });

    LoadThread.detach();
}

void CoDPackageCache::LoadPackageAsync(const std::string& FilePath)
{
    // Same race as above, flag before the thread exists
    BeginLoadingState();

    std::thread LoadThread([FilePath, this]
    {
        this->LoadPackage(FilePath);
        // Set the loaded state, this is only called on single entries
        this->SetLoadedState();
    });

    LoadThread.detach();
}

void CoDPackageCache::BeginLoadingState()
{
    std::lock_guard<std::mutex> Lock(CacheMutex);

    // Mark as loading before any waiter can observe us
    CacheLoading = true;
    HasLoaded = false;
}

void CoDPackageCache::WaitForPackageCacheLoad()
{
    while (this->IsCacheLoading())
    {
        Sleep(100);
    }
}