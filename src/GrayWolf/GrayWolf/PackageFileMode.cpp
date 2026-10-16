#include "stdafx.h"

#include "ExportWriters.h"

// The `file` subcommand's use case (PackageFileMode.h).

#include "PackageFileMode.h"

#include "CoDAssets.h"
#include "ICoDGame.h"
#include "LiveExport.h"
#include "CoDXConverter.h"
#include "FileSystems.h"
#include "Strings.h"
#include "Image.h"
#include "SettingsManager.h"

#include "IWDCache.h"
#include "IPAKCache.h"
#include "XPAKCache.h"
#include "SABCache.h"
#include "XPAKSupport.h"
#include "IPAKSupport.h"
#include "IWDSupport.h"
#include "SABSupport.h"

#include <algorithm>
#include <atomic>
#include <filesystem>
#include <iostream>

namespace
{
    std::string LastExportPath;

    // exported_files/<the game's folder>/<the kind's folder>
    std::string BuildExportPath(const CoDAsset_t* Asset)
    {
        auto ApplicationPath = (std::filesystem::path(FileSystems::GetApplicationPath()) / "exported_files").string();

        if (CoDAssets::Game != nullptr)
        {
            const std::string FolderName = CoDAssets::Game->GetExportFolderName();
            if (!FolderName.empty())
                ApplicationPath = (std::filesystem::path(ApplicationPath) / FolderName).string();
        }

        switch (Asset->AssetType)
        {
        case WraithAssetType::Image:
            ApplicationPath = (std::filesystem::path(ApplicationPath) / "ximages").string();
            break;
        case WraithAssetType::Sound:
            ApplicationPath = (std::filesystem::path(ApplicationPath) / "sounds").string();
            if (SettingsManager::GetSetting("keepsndpath", "true") == "true" && !((const CoDSound_t*)Asset)->FullPath.empty())
                ApplicationPath = (std::filesystem::path(ApplicationPath) / ((const CoDSound_t*)Asset)->FullPath).string();
            break;
        default:
            break;
        }

        return ApplicationPath;
    }

    ExportGameResult ExportAsset(const CoDAsset_t* Asset)
    {
        std::error_code Ec;
        auto ExportPath = BuildExportPath(Asset);
        std::filesystem::create_directories(ExportPath, Ec);

        auto ImageExtension = "." + Strings::ToLower(SettingsManager::GetSetting("exportimg"));
        auto SoundExtension = ExportWriters::SoundExtension(ExportWriters::SoundFormatSetting());
        const bool SoundsAsRead = CoDAssets::Game != nullptr && CoDAssets::Game->SoundsAsRead();

        auto Result = ExportGameResult::Success;

        try
        {
            // A package file holds images (.xpak, .ipak, .iwd) or sounds (.sabs, .sabl), nothing else
            switch (Asset->AssetType)
            {
            case WraithAssetType::Image: Result = LiveExport::ExportImage((const CoDImage_t*)Asset, ExportPath, ImageExtension); break;
            case WraithAssetType::Sound: Result = LiveExport::ExportSound((const CoDSound_t*)Asset, ExportPath, SoundsAsRead ? ".wav" : SoundExtension); break;
            default: break;
            }
        }
        catch (...)
        {
            Result = ExportGameResult::UnknownError;
            CoDAssets::Log->error("An exception occurred while exporting asset: {0}", Asset->AssetName);
#if _DEBUG
            std::cout << "Error: An exception occurred while exporting asset:" << Asset->AssetName.c_str() << "\n";
#endif // _DEBUG
        }

        return Result;
    }

    LoadGameFileResult LoadFile(const std::string& FilePath)
    {
        auto& Assets = CoDAssets::GameAssets;
        auto& Packages = CoDAssets::GamePackageCache;
        Assets.reset(new AssetPool());

        auto LoadResult = false;

        auto FileExt = Strings::ToLower(FileSystems::GetExtension(FilePath));

        // Each parser names the game the file belongs to (CoDAssets::Game), for its readers
        if (FileExt == ".xpak")
        {
            LoadResult = XPAKSupport::ParseXPAK(FilePath);

            if (LoadResult)
            {
                Packages = std::make_unique<XPAKCache>();
                Packages->LoadPackageAsync(FilePath);
            }
        }
        else if (FileExt == ".ipak")
        {
            LoadResult = IPAKSupport::ParseIPAK(FilePath);

            if (LoadResult)
            {
                Packages = std::make_unique<IPAKCache>();
                Packages->LoadPackageAsync(FilePath);

                CoDAssets::Game = LiveGames::OwnerOf(PackageMark::Container::Ipak, 0);
            }
        }
        else if (FileExt == ".iwd")
        {
            LoadResult = IWDSupport::ParseIWD(FilePath);

            if (LoadResult)
            {
                Packages = std::make_unique<IWDCache>();
                Packages->LoadPackageAsync(FilePath);

                // The game whose folder the file is in (the one claiming any .iwd otherwise)
                CoDAssets::Game = LiveGames::OwnerOf(PackageMark::Container::Iwd, 0, 0, Strings::ToLower(FileSystems::GetDirectoryName(FilePath)));
            }
        }
        else if (FileExt == ".sabs" || FileExt == ".sabl")
        {
            LoadResult = SABSupport::ParseSAB(FilePath);

            if (LoadResult)
            {
                Packages = std::make_unique<SABCache>();
                Packages->LoadPackageCacheAsync(FilePath);
            }
        }
        else
        {
            return LoadGameFileResult::UnknownError;
        }

        if (LoadResult)
        {
            auto sortMethod = AssetCompareMethodHelper::CalculateCompareMethod(SettingsManager::GetSetting("assetsortmethod", "Name"));

            if (sortMethod != AssetCompareMethod::None)
            {
                std::stable_sort(Assets->LoadedAssets.begin(), Assets->LoadedAssets.end(), [sortMethod](const CoDAsset_t* lhs, const CoDAsset_t* rhs)
                {
                    return lhs->Compare(rhs, sortMethod);
                });
            }

            return LoadGameFileResult::Success;
        }

        Assets.reset();

        return LoadGameFileResult::InvalidFile;
    }
}

LoadGameFileResult PackageFileMode::Load(const std::string& FilePath)
{
    // Nothing from a previous load stays
    CoDAssets::CleanUpGame();

    return LoadFile(FilePath);
}

const std::string& PackageFileMode::LatestExportPath()
{
    return LastExportPath;
}

uint32_t PackageFileMode::Export(const std::vector<uint32_t>* Selection, uint32_t Threads)
{
    std::vector<CoDAsset_t*> Assets;
    const auto& Loaded = CoDAssets::GameAssets->LoadedAssets;
    if (Selection == nullptr)
        Assets.assign(Loaded.begin(), Loaded.end());
    else
        for (const auto Index : *Selection)
            Assets.push_back(Loaded[Index]);

    // We must wait until the package cache is fully loaded before exporting
    if (CoDAssets::GamePackageCache != nullptr)
        CoDAssets::GamePackageCache->WaitForPackageCacheLoad();

    // The single asset's folder, or the first asset's kind's folder
    if (Assets.size() == 1)
        LastExportPath = BuildExportPath(Assets[0]);
    else if (Assets.size() > 0)
        LastExportPath = FileSystems::GetDirectoryName(BuildExportPath(Assets[0]));

    // Start each export with a clean image cache, the output folder may have been
    // emptied or moved since the last one
    LiveExport::ForgetWrittenImages();

    std::atomic<uint32_t> AssetIndex = 0;
    std::atomic<uint32_t> Exported = 0;
    const uint32_t AssetsToConvert = (uint32_t)Assets.size();

    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    auto NumberOfCores = SystemInfo.dwNumberOfProcessors;

#if _DEBUG
    auto DegreeOfConverter = 1;
#else
    // Explicit override wins, else NumberOfCores-2 (the export is CPU-bound, encoding-dominated).
    auto DegreeOfConverter = (Threads > 0)
        ? std::clamp<uint32_t>(Threads, 1, 64)
        : std::clamp<uint32_t>((NumberOfCores > 2) ? NumberOfCores - 2 : 1, 1, 16);
#endif
    CoDXConverter([&AssetIndex, &Exported, &Assets, AssetsToConvert]
    {
        Image::SetupConversionThread();

        while (AssetIndex < AssetsToConvert)
        {
            auto AssetToConvert = AssetIndex++;

            if (AssetToConvert >= Assets.size())
                continue;

            auto& Asset = Assets[AssetToConvert];
#ifndef DEBUG
            if (Asset->AssetStatus != WraithAssetStatus::Placeholder)
#endif
            {
                Asset->AssetStatus = WraithAssetStatus::Processing;

                auto Result = ExportGameResult::UnknownError;

                try
                {
                    CoDAssets::Log->info("Exporting: {0}...", Asset->AssetName);
                    Result = ExportAsset(Asset);
                    CoDAssets::Log->info("Successfully exported: {0}", Asset->AssetName);
                }
                catch (std::exception& ex)
                {
                    CoDAssets::Log->error(ex.what());
                }

                Asset->AssetStatus = (Result == ExportGameResult::Success) ? WraithAssetStatus::Exported : WraithAssetStatus::Error;
            }

            Exported++;
        }

        Image::DisableConversionThread();
    }, DegreeOfConverter);

    return Exported;
}
