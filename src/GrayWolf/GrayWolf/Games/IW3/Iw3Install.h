#pragma once

// An install of the IW3 family's layout (CoD4, World at War, Black Ops, Black Ops II):
// zone\<folder>\x.ff, the .iwd archives in main\.

#include <memory>
#include <string>

class IWDCache;

namespace Iw3Family
{
    // zone\<folder>\x.ff: the install is two folders above the zone's folder (or Given)
    std::string InstallOf(const std::string& FastFile, const std::string& Given);

    // The install's .iwd archives (main\*.iwd), indexed once per install for the run
    std::shared_ptr<IWDCache> IwdPackagesOf(const std::string& Install);
}
