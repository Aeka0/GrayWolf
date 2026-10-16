#include "stdafx.h"

#include "IPAKCache.h"

#include "Compression.h"
#include "BinaryReader.h"
#include "Strings.h"

#include "DBGameFiles.h"

#include <filesystem>

IPAKCache::IPAKCache()
{
}

IPAKCache::~IPAKCache()
{
}

void IPAKCache::LoadPackageCache(const std::string& BasePath)
{
    CoDPackageCache::LoadPackageCache(BasePath);

    std::error_code Ec;
    for (const auto& File : std::filesystem::directory_iterator(BasePath, Ec))
    {
        if (File.is_regular_file(Ec) && Strings::ToLower(File.path().extension().string()) == ".ipak")
            this->LoadPackage(File.path().string());
    }

    this->SetLoadedState();
}

bool IPAKCache::LoadPackage(const std::string& FilePath)
{
    CoDPackageCache::LoadPackage(FilePath);

    auto PackageIndex = (uint32_t)PackageFilePaths.size();

    auto Reader = BinaryReader();
    Reader.Open(FilePath);

    auto Header = Reader.Read<BO2IPakHeader>();

    if (Header.Magic == 0x4950414b)
    {
        auto IPAKEntriesSegment = BO2IPakSegment();
        auto IPAKDataSegment = BO2IPakSegment();

        for (uint32_t i = 0; i < Header.SegmentCount; i++)
        {
            auto Segment = Reader.Read<BO2IPakSegment>();

            if (Segment.Type == 1) { IPAKEntriesSegment = Segment; }
            if (Segment.Type == 2) { IPAKDataSegment = Segment; }
        }

        if (IPAKEntriesSegment.Type == 1)
        {
            Reader.SetPosition(IPAKEntriesSegment.Offset);

            for (uint32_t i = 0; i < IPAKEntriesSegment.EntryCount; i++)
            {
                auto Entry = Reader.Read<BO2IPakEntry>();

                PackageCacheObject NewObject;
                NewObject.Offset = Entry.Offset;
                NewObject.CompressedSize = Entry.Size;
                NewObject.UncompressedSize = IPAKDataSegment.Offset;
                NewObject.PackageFileIndex = PackageIndex;

                CacheObjects.insert(std::make_pair(Entry.Key, NewObject));
                KeyByOffset.insert(std::make_pair((uint64_t)Entry.Offset, Entry.Key));
            }
        }

        PackageFilePaths.push_back(FilePath);
    }

    return true;
}

std::unique_ptr<uint8_t[]> IPAKCache::ExtractPackageObject(uint64_t CacheID, uint32_t& ResultSize)
{
    if (CacheObjects.find(CacheID) != CacheObjects.end())
    {
        // Uncompressed size = offset of data segment!
        auto& CacheInfo = CacheObjects[CacheID];
        auto& IPAKFileName = PackageFilePaths[CacheInfo.PackageFileIndex];

        auto Reader = BinaryReader();
        Reader.Open(IPAKFileName);

        Reader.SetPosition(CacheInfo.Offset + CacheInfo.UncompressedSize);

        uint64_t DataRead = 0;
        uint64_t TotalDataSize = 0;

        auto DataTemporaryBuffer = new int8_t[0x1000000];

        while (DataRead < CacheInfo.CompressedSize)
        {
            auto BlockHeader = Reader.Read<BO2IPakDataHeader>();

            for (uint32_t i = 0; i < BlockHeader.Count; i++)
            {
                uint64_t BlockSize = (BlockHeader.Commands[i] & 0xFFFFFF);
                uint32_t CompressedFlag = (BlockHeader.Commands[i] >> 24);

                uint64_t CurrentPosition = Reader.GetPosition();
                
                // Block type: 1 = compressed, 0 = raw data, anything else = skip over
                if (CompressedFlag == 0x1)
                {
                    uint64_t ReadSize = 0;
                    auto DataBlock = Reader.Read(BlockSize, ReadSize);

                    if (DataBlock != nullptr)
                    {
                        auto Result = Compression::DecompressLZO1XBlock((const int8_t*)DataBlock, DataTemporaryBuffer + TotalDataSize, (uint32_t)BlockSize, 0x1000000);

                        TotalDataSize += Result;

                        delete[] DataBlock;
                    }
                }
                else if (CompressedFlag == 0x0)
                {
                    uint64_t ReadSize = 0;
                    auto DataBlock = Reader.Read(BlockSize, ReadSize);

                    if (DataBlock != nullptr)
                    {
                        std::memcpy(DataTemporaryBuffer + TotalDataSize, DataBlock, BlockSize);

                        TotalDataSize += BlockSize;

                        delete[] DataBlock;
                    }
                }
                else
                {
                    // As far as we care, any other flag value is padding (0xCF is one of them)
                    Reader.Advance(BlockSize);
                }
                
                // We must append the block size and pad it properly (If it's the last block)
                uint64_t NextSegmentOffset = 0;
                uint64_t TotalBlockSize = 0;

                if ((i + 1) < BlockHeader.Count)
                {
                    TotalBlockSize = BlockSize;
                    NextSegmentOffset = (CurrentPosition + BlockSize);
                }
                else
                {
                    NextSegmentOffset = (((CurrentPosition + BlockSize) + 0x7F) & 0xFFFFFFFFFFFFF80);
                    TotalBlockSize += (NextSegmentOffset - CurrentPosition);
                }

                // Includes padding
                DataRead += TotalBlockSize;

                Reader.SetPosition(NextSegmentOffset);
            }

            DataRead += sizeof(BO2IPakDataHeader);
        }

        auto ResultBuffer = std::make_unique<uint8_t[]>((uint32_t)TotalDataSize);
        std::memcpy(ResultBuffer.get(), DataTemporaryBuffer, TotalDataSize);

        delete[] DataTemporaryBuffer;

        ResultSize = (uint32_t)TotalDataSize;

        return ResultBuffer;
    }

    ResultSize = 0;

    return nullptr;
}
