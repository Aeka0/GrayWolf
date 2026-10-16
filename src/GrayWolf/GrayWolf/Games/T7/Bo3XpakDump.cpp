#include "stdafx.h"

#include "Bo3XpakDump.h"

#include "XPAKCache.h"

#include <filesystem>
#include <format>
#include <fstream>

namespace
{
    // Subclassed to reach the protected CacheObjects table and to call LoadPackage() on explicit
    // file paths (LoadPackageCache scans a whole directory).
    class Bo3XpakEnumerator : public XPAKCache
    {
    public:
        using XPAKCache::CacheObjects;
        using XPAKCache::LoadPackage;
    };
}

size_t Bo3DumpXpakEntries(const std::vector<std::string>& XpakFiles, const std::string& OutDir,
                           uint64_t MinSizeBytes, std::string& Report)
{
    std::error_code Ec;
    Bo3XpakEnumerator Cache;

    size_t LoadedFiles = 0;
    for (auto& Path : XpakFiles)
    {
        if (Cache.LoadPackage(Path))
            LoadedFiles++;
    }

    std::filesystem::create_directories(OutDir, Ec);

    size_t Written = 0;
    std::string Lines;

    for (auto& Kvp : Cache.CacheObjects)
    {
        const uint64_t Key = Kvp.first;
        const uint64_t CompressedSize = Kvp.second.CompressedSize;

        if (CompressedSize < MinSizeBytes)
            continue;

        // Generous decompressed-size guess: an LZ4/Oodle block can only grow moderately, and an
        // encrypted payload is expected to be stored close to 1:1 (raw copy blocks) - 4x the
        // compressed size, floored/capped to a sane range, comfortably covers both.
        uint64_t Guess = CompressedSize * 4;
        if (Guess < 8ull * 1024 * 1024) Guess = 8ull * 1024 * 1024;
        if (Guess > 300ull * 1024 * 1024) Guess = 300ull * 1024 * 1024;

        uint32_t ResultSize = 0;
        auto Data = Cache.ExtractPackageObject(Key, (int32_t)Guess, ResultSize);

        if (!Data || ResultSize == 0)
        {
            Lines += std::format("SKIP  key=0x{:016x} compressed={} (extract failed)\n",
                                      (unsigned long long)Key, (unsigned long long)CompressedSize);
            continue;
        }

        const std::string OutPath = (std::filesystem::path(OutDir) / std::format("{:016x}.bin", (unsigned long long)Key)).string();

        std::ofstream F(OutPath, std::ios::binary);
        F.write((const char*)Data.get(), ResultSize);
        F.close();

        Lines += std::format("WROTE key=0x{:016x} compressed={} decompressed={} -> {}\n",
                                  (unsigned long long)Key, (unsigned long long)CompressedSize,
                                  ResultSize, OutPath.c_str());
        Written++;
    }

    Report = std::format(
        "Loaded {}/{} xpak file(s), {} total entries, wrote {} entries >= {} bytes to {}\n\n",
        LoadedFiles, XpakFiles.size(), Cache.CacheObjects.size(), Written,
        (unsigned long long)MinSizeBytes, OutDir.c_str()) + Lines;

    return Written;
}
