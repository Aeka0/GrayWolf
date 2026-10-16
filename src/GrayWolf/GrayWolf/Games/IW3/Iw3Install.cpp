#include "stdafx.h"

#include "Games/IW3/Iw3Install.h"

#include "ExportNaming.h"
#include "IWDCache.h"

#include <filesystem>
#include <map>
#include <mutex>

namespace fs = std::filesystem;

namespace Iw3Family
{
    std::string InstallOf(const std::string& FastFile, const std::string& Given)
    {
        if (!Given.empty()) return Given;
        const fs::path P = fs::path(FastFile).parent_path();
        if (ExportNaming::Lower(P.filename().string()) == "zone") return P.parent_path().string();
        return P.parent_path().parent_path().string();
    }

    std::shared_ptr<IWDCache> IwdPackagesOf(const std::string& Install)
    {
        static std::mutex Lock;
        static std::map<std::string, std::shared_ptr<IWDCache>> Packages;
        std::lock_guard<std::mutex> G(Lock);
        auto& P = Packages[Install];
        if (!P)
        {
            P = std::make_shared<IWDCache>();
            P->LoadPackageCache((fs::path(Install) / "main").string());
        }
        return P;
    }
}
