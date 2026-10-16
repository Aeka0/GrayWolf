#pragma once

#include <string>

#include "CoDPackageCache.h"

class SABCache : public CoDPackageCache
{
public:
    SABCache();
    virtual ~SABCache();

    virtual void LoadPackageCache(const std::string& BasePath);
};