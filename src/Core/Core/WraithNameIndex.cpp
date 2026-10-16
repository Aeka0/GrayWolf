#include "stdafx.h"

#include "WraithNameIndex.h"

#include "BinaryReader.h"
#include "BinaryWriter.h"
#include "MemoryReader.h"
#include "Compression.h"

#include <filesystem>

WraithNameIndex::WraithNameIndex()
{
}

WraithNameIndex::WraithNameIndex(const std::string& FilePath)
{
    this->LoadIndex(FilePath);
}

void WraithNameIndex::SaveIndex(const std::string& FilePath)
{
    auto Writer = BinaryWriter();
    Writer.Create(FilePath);

    Writer.Write<uint32_t>(0x20494E57);
    Writer.Write<uint16_t>(0x1);

    Writer.Write<uint32_t>((uint32_t)NameDatabase.size());

    uint32_t PackageSize = 0;

    for (auto& KeyValue : NameDatabase)
    {
        // Add 8 for key and then string size
        PackageSize += 8 + (uint32_t)(KeyValue.second.size() + 1);
    }

    auto UnpackedData = new int8_t[PackageSize];

    uint32_t DataOffset = 0;

    for (auto& KeyValue : NameDatabase)
    {
        std::memcpy(UnpackedData + DataOffset, &KeyValue.first, 8);
        DataOffset += 8;
        std::memcpy(UnpackedData + DataOffset, KeyValue.second.c_str(), KeyValue.second.size() + 1);
        DataOffset += (uint32_t)(KeyValue.second.size() + 1);
    }

    // Allocate a buffer for compressed (At least 1 / 2 of size in addition)
    auto PackedData = new int8_t[PackageSize + (PackageSize / 2)];

    auto ResultSize = Compression::CompressLZ4Block(UnpackedData, PackedData, PackageSize, PackageSize + (PackageSize / 2));

    delete[] UnpackedData;

    if (ResultSize > 0)
    {
        Writer.Write<uint32_t>(ResultSize);
        Writer.Write<uint32_t>(PackageSize);

        Writer.Write(PackedData, ResultSize);
    }

    delete[] PackedData;
}

void WraithNameIndex::LoadIndex(const std::string& LoadIndex, uint64_t KeyMask)
{
    std::error_code Ec;
    if (std::filesystem::is_regular_file(LoadIndex, Ec))
    {
        auto Reader = BinaryReader();
        Reader.Open(LoadIndex);

        // Verify magic ('WNI ')
        if (Reader.Read<uint32_t>() == 0x20494E57)
        {
            // Skip over version (Should be 0x1)
            Reader.Advance(2);

            auto EntryCount = Reader.Read<uint32_t>();

            auto Packed = Reader.Read<uint32_t>();
            auto Unpacked = Reader.Read<uint32_t>();

            uint64_t CompressedResult = 0;
            auto CompressedBuffer = Reader.Read(Packed, CompressedResult);

            if (CompressedBuffer != nullptr)
            {
                auto Decompressed = new int8_t[Unpacked];

                auto DecompressResult = Compression::DecompressLZ4Block(CompressedBuffer, Decompressed, Packed, Unpacked);

                if (DecompressResult > 0)
                {
                    auto MemReader = MemoryReader(Decompressed, DecompressResult);

                    // Loop and read entries (room made once: no rehash per growth)
                    NameDatabase.reserve(NameDatabase.size() + EntryCount);
                    for (uint32_t i = 0; i < EntryCount; i++)
                    {
                        auto Key = MemReader.Read<uint64_t>() & KeyMask;
                        auto Value = MemReader.ReadNullTerminatedString();

                        // Add the entry, a later one replacing it
                        NameDatabase.insert_or_assign(Key, std::move(Value));
                    }
                }
                else
                {
                    delete[] Decompressed;
                }

                delete[] CompressedBuffer;
            }
        }
    }
}