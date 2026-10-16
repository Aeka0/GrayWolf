#include "stdafx.h"

#include "PAKCache.h"

#include "CoDFileHandle.h"
#include "Compression.h"

PAKCache::PAKCache()
{
}

PAKCache::~PAKCache()
{
}

void PAKCache::LoadPackageCache(const std::string& BasePath)
{
    CoDPackageCache::LoadPackageCache(BasePath);

    this->PackageFilesPath = BasePath;

    this->SetLoadedState();
}

std::unique_ptr<uint8_t[]> PAKCache::ExtractPackageObject(const std::string& PackageName, const uint64_t AssetOffset, const uint64_t AssetSize, size_t& ResultSize)
{
    // AW, MWR image package
    ResultSize = 0;

    auto Reader = CoDFileHandle(FileSystem->OpenFile(PackageName, "r"), FileSystem.get());;

    Reader.Seek(AssetOffset, SEEK_SET);

    auto TotalUncompressedSize = Reader.Read<uint64_t>();
    Reader.Seek(4, SEEK_CUR);

    auto ResultBuffer = std::make_unique<uint8_t[]>((uint32_t)TotalUncompressedSize);

    uint64_t TotalReadUncompressedSize = 0;

    while (TotalUncompressedSize != TotalReadUncompressedSize)
    {
        auto CompressedSize = Reader.Read<uint32_t>();
        auto UncompressedSize = Reader.Read<uint32_t>();
        auto CompressedData = Reader.Read(CompressedSize);

        if (CompressedData != nullptr)
        {
            auto DecompressResult = Compression::DecompressLZ4Block((const int8_t*)CompressedData.get(), (int8_t*)ResultBuffer.get() + TotalReadUncompressedSize, CompressedSize, UncompressedSize);
        }

        // Skip over padding
        auto CurrentPosition = Reader.Tell();
        Reader.Seek(((Reader.Tell() + 0x3) & 0xFFFFFFFFFFFFFFC), SEEK_SET);

        TotalReadUncompressedSize += UncompressedSize;
    }

    ResultSize = (uint32_t)TotalUncompressedSize;

    return ResultBuffer;
}

