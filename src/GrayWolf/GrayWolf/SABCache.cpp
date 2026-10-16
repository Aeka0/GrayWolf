#include "stdafx.h"

#include "SABCache.h"

SABCache::SABCache()
{
}

SABCache::~SABCache()
{
}

void SABCache::LoadPackageCache(const std::string& BasePath)
{
    CoDPackageCache::LoadPackageCache(BasePath);

    this->PackageFilesPath = BasePath;

    this->SetLoadedState();
}