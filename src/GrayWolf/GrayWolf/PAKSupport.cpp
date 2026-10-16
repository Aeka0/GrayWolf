#include "stdafx.h"

#include "PAKSupport.h"

#include "BinaryReader.h"
#include "Compression.h"

#include <filesystem>

std::unique_ptr<uint8_t[]> PAKSupport::GhostsExtractImagePackage(const std::string& PackageName, uint64_t AssetOffset, uint64_t AssetSize, uint32_t& ResultSize)
{
    std::error_code Ec;
    ResultSize = 0;

    if (std::filesystem::is_regular_file(PackageName, Ec))
    {
        auto Reader = BinaryReader();
        Reader.Open(PackageName);

        Reader.SetPosition(AssetOffset);
        
        uint64_t ReadSize = 0;
        auto CompressedData = Reader.Read(AssetSize, ReadSize);

        if (CompressedData != nullptr)
        {
            auto TempWorkingBuffer = new int8_t[30000000];

            auto Result = Compression::DecompressZLibBlock((const int8_t*)CompressedData, TempWorkingBuffer, (uint32_t)AssetSize, 30000000);            
            
            delete[] CompressedData;

            auto ResultBuffer = std::make_unique<uint8_t[]>(Result);
            std::memcpy(ResultBuffer.get(), TempWorkingBuffer, Result);

            delete[] TempWorkingBuffer;

            ResultSize = Result;

            return ResultBuffer;
        }
    }

    return nullptr;
}

namespace
{
    // An LZ4 pak entry at Start: its u64 uncompressed size and 4 bytes, then chunks {u32
    // compressed size, u32 uncompressed size, ChunkPadding bytes, data}, each 4-byte aligned
    std::unique_ptr<uint8_t[]> ExtractLZ4Chunks(const std::string& PackageName, uint64_t Start, uint32_t ChunkPadding, uint32_t& ResultSize)
    {
        std::error_code Ec;
        ResultSize = 0;

        if (!std::filesystem::is_regular_file(PackageName, Ec))
            return nullptr;

        auto Reader = BinaryReader();
        Reader.Open(PackageName);

        Reader.SetPosition(Start);

        auto TotalUncompressedSize = Reader.Read<uint64_t>();
        Reader.Advance(4);

        auto ResultBuffer = std::make_unique<uint8_t[]>((uint32_t)TotalUncompressedSize);

        uint64_t TotalReadUncompressedSize = 0;

        while (TotalUncompressedSize != TotalReadUncompressedSize)
        {
            auto CompressedSize = Reader.Read<uint32_t>();
            auto UncompressedSize = Reader.Read<uint32_t>();

            if (ChunkPadding > 0)
                Reader.Advance(ChunkPadding);

            uint64_t ReadCompress = 0;
            auto CompressedData = Reader.Read(CompressedSize, ReadCompress);

            if (CompressedData != nullptr)
            {
                Compression::DecompressLZ4Block((const int8_t*)CompressedData, (int8_t*)ResultBuffer.get() + TotalReadUncompressedSize, CompressedSize, UncompressedSize);

                delete[] CompressedData;
            }

            auto CurrentPosition = Reader.GetPosition();
            Reader.SetPosition(((CurrentPosition + 0x3) & 0xFFFFFFFFFFFFFFC));

            TotalReadUncompressedSize += UncompressedSize;
        }

        ResultSize = (uint32_t)TotalUncompressedSize;

        return ResultBuffer;
    }
}

std::unique_ptr<uint8_t[]> PAKSupport::AWExtractImagePackage(const std::string& PackageName, uint64_t AssetOffset, uint64_t AssetSize, uint32_t& ResultSize)
{
    return ExtractLZ4Chunks(PackageName, AssetOffset, 0, ResultSize);
}

std::unique_ptr<uint8_t[]> PAKSupport::IWExtractImagePackage(const std::string& PackageName, uint64_t AssetOffset, uint64_t AssetSize, uint32_t& ResultSize)
{
    // IW: the entry starts 4 bytes in, its chunks have 4 more bytes after their sizes
    return ExtractLZ4Chunks(PackageName, AssetOffset + 4, 4, ResultSize);
}
