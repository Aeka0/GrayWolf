#include "stdafx.h"

#include "IPAKSupport.h"

#include "BinaryReader.h"
#include "WraithNameIndex.h"
#include "FileSystems.h"

#include "CoDAssets.h"
#include "DBGameFiles.h"

#include <filesystem>
#include <format>

bool IPAKSupport::ParseIPAK(const std::string& FilePath)
{
    auto Reader = BinaryReader();
    Reader.Open(FilePath);

    auto Header = Reader.Read<BO2IPakHeader>();

    if (Header.Magic == 0x4950414b)
    {
        WraithNameIndex IPAKNames((std::filesystem::path(FileSystems::GetApplicationPath()) / "package_index\\bo2_ipak.wni").string());
        
        auto IPAKEntriesSegment = BO2IPakSegment();
        auto IPAKDataSegment = BO2IPakSegment();

        for (uint32_t i = 0; i < Header.SegmentCount; i++)
        {
            // a count past the end of the file is a corrupt header: stop where the file does
            if (Reader.GetPosition() + sizeof(BO2IPakSegment) > Reader.GetLength())
                break;
            auto Segment = Reader.Read<BO2IPakSegment>();

            if (Segment.Type == 1) { IPAKEntriesSegment = Segment; }
            if (Segment.Type == 2) { IPAKDataSegment = Segment; }
        }

        if (IPAKEntriesSegment.Type == 1)
        {
            Reader.SetPosition(IPAKEntriesSegment.Offset);

            for (uint32_t i = 0; i < IPAKEntriesSegment.EntryCount; i++)
            {
                if (Reader.GetPosition() + sizeof(BO2IPakEntry) > Reader.GetLength())
                    break;
                auto Entry = Reader.Read<BO2IPakEntry>();

                std::string AssetName = "";
                if (IPAKNames.NameDatabase.find(Entry.Key) != IPAKNames.NameDatabase.end())
                {
                    AssetName = IPAKNames.NameDatabase[Entry.Key];
                }
                else
                {
                    AssetName = std::format("_{:x}", Entry.Key);
                }

                auto LoadedImage = new CoDImage_t();
                LoadedImage->AssetName = AssetName;
                LoadedImage->AssetPointer = Entry.Key;
                LoadedImage->Width = 0;
                LoadedImage->Height = 0;
                LoadedImage->AssetStatus = WraithAssetStatus::Loaded;
                LoadedImage->IsFileEntry = true;

                CoDAssets::GameAssets->LoadedAssets.push_back(LoadedImage);
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}
