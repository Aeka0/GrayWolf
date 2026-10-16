#include "stdafx.h"

#include "XPTOCCache.h"

#include "BinaryReader.h"
#include "Hashing.h"
#include "PAKSupport.h"

#include "DBGameFiles.h"

#include <filesystem>
#include <format>

XPTOCCache::XPTOCCache()
{
}

XPTOCCache::~XPTOCCache()
{
}

void XPTOCCache::LoadPackageCache(const std::string& BasePath)
{
    CoDPackageCache::LoadPackageCache(BasePath);

    this->PackageFilesPath = BasePath;

    // Only 'xpakfile.toc' is loaded
    this->LoadPackage((std::filesystem::path(BasePath) / "xpakfile.toc").string());

    this->SetLoadedState();
}

bool XPTOCCache::LoadPackage(const std::string& FilePath)
{
    CoDPackageCache::LoadPackage(FilePath);

    auto Reader = BinaryReader();
    Reader.Open(FilePath);

    auto Header = Reader.Read<WWIIXPTocHeader>();

    if (Header.Magic == 0x3030336966663253)
    {
        for (uint32_t i = 0; i < Header.EntriesCount; i++)
        {
            auto Entry = Reader.Read<WWIIXPTocEntry>();

            const char HexMap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

            std::string HexBuffer(32, ' ');
            for (int h = 0; h < 16; h++)
            {
                HexBuffer[h * 2] = HexMap[(Entry.Hash[h] & 0xF0) >> 4];
                HexBuffer[h * 2 + 1] = HexMap[Entry.Hash[h] & 0x0F];
            }

            auto Key = Hashing::HashXXHashString(HexBuffer);

            PackageCacheObject NewObject;
            NewObject.Offset = Entry.Offset;
            NewObject.CompressedSize = Entry.Size;
            NewObject.UncompressedSize = Entry.Size;
            NewObject.PackageFileIndex = Entry.PackageIndex;

            CacheObjects.insert(std::make_pair(Key, NewObject));
        }
    }

    return true;
}

std::unique_ptr<uint8_t[]> XPTOCCache::ExtractPackageObject(uint64_t CacheID, uint32_t& ResultSize)
{
    if (CacheObjects.find(CacheID) != CacheObjects.end())
    {
        // Uncompressed size = final chunk size
        auto& CacheInfo = CacheObjects[CacheID];
        auto FilePath = (std::filesystem::path(this->PackageFilesPath) / std::format("xpakfile{}.pak", CacheInfo.PackageFileIndex)).string();

        return PAKSupport::AWExtractImagePackage(FilePath, CacheInfo.Offset, CacheInfo.UncompressedSize, ResultSize);
    }

    ResultSize = 0;

    return nullptr;
}