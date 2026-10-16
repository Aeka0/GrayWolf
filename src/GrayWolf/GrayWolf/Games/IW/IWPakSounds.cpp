#include "stdafx.h"

#include "Games/IW/IWPakSounds.h"

#include "CoDAssets.h"
#include "ExportNaming.h"
#include "ExportWriters.h"
#include "SettingsManager.h"
#include "Strings.h"

#include <filesystem>
#include <format>
#include <set>

namespace fs = std::filesystem;

namespace IWPakSounds
{
    PackagesInUse::PackagesInUse(CoDPackageCache* Packages) : Saved(std::move(CoDAssets::GamePackageCache))
    {
        CoDAssets::GamePackageCache = std::unique_ptr<CoDPackageCache>(Packages);
    }

    PackagesInUse::~PackagesInUse()
    {
        CoDAssets::GamePackageCache.release();
        CoDAssets::GamePackageCache = std::move(Saved);
    }

    std::string Extension()
    {
        return ExportWriters::SoundExtension(ExportWriters::SoundFormatSetting());
    }

    void Write(std::vector<CoDAsset_t*>& Found, CoDPackageCache* Packages, const DiskGame::Context& C, DiskGame::Totals& T,
               const std::string& Zone, const Counters& Counter, const WriteOptions& Options)
    {
        auto& Kt = T.Kinds[DiskGame::Kind::Sound];
        const PackagesInUse Using(Packages);
        const std::string Extension = IWPakSounds::Extension();
        const std::string Dir = C.KindDir(DiskGame::Kind::Sound);
        const auto Success = static_cast<int>(ExportGameResult::Success);
        std::set<std::string> Names;
        bool Going = true;   // until cancelled; the rest is only freed
        for (size_t N = 0; N < Found.size(); N++)
        {
            auto* Sound = static_cast<CoDSound_t*>(Found[N]);
            Going = Going && C.Advance(N, Found.size(), Sound->AssetName);
            Sound->AssetName = ExportNaming::SafeFileName(Sound->AssetName);
            if (!Going) {}
            else if (!Sound->IsFileEntry && Sound->AssetSize == 0) Kt.References++;
            else if (C.Wants(DiskGame::Kind::Sound, Sound->AssetName) && (!Options.OnePerName || Names.insert(Sound->AssetName = Strings::ToLower(Sound->AssetName)).second))
            {
                Kt.Seen++;
                Counter(Sound->IsFileEntry ? "sounds_streamed" : "sounds_loaded")++;
                std::string Crash;
                const int Code = ExportWriters::ExportSound(*Sound, Dir, Extension, Crash);
                std::error_code Ec;
                const fs::path File = fs::path(Dir) / (Sound->AssetName + Extension);
                if (Code == Success && Options.EmptyFileFails && fs::exists(File, Ec) && fs::file_size(File, Ec) == 0)
                    T.Fail(Zone, DiskGame::Kind::Sound, Sound->AssetName, std::format("empty file ({}, package {}, offset {:x}, size {})",
                        Sound->IsFileEntry ? "pak" : "zone", (unsigned)Sound->PackageIndex, (unsigned long long)Sound->AssetPointer, (unsigned long long)Sound->AssetSize));
                else if (Code == Success) Kt.Written++;
                else T.Fail(Zone, DiskGame::Kind::Sound, Sound->AssetName, std::format("code {} {} ({}, package {}, offset {:x}, size {}, localized {})",
                    Code, Crash.c_str(), Sound->IsFileEntry ? "streamed" : "loaded", (unsigned)Sound->PackageIndex,
                    (unsigned long long)Sound->AssetPointer, (unsigned long long)Sound->AssetSize, Sound->IsLocalized ? 1 : 0));
            }
            delete Sound;
        }
        Found.clear();
    }
}
