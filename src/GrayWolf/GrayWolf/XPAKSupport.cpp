#include "stdafx.h"

#include "XPAKSupport.h"

#include "MemoryReader.h"
#include "BinaryReader.h"
#include "FileSystems.h"
#include "Strings.h"
#include "SettingsManager.h"
#include "WraithNameIndex.h"

#include "CoDAssets.h"
#include "ICoDGame.h"
#include "CoDRawImageTranslator.h"
#include "DBGameFiles.h"

#include <charconv>
#include <filesystem>
#include <format>

struct XImageMipInfo
{
    uint32_t Width;
    uint32_t Height;
    std::string Format;
    uint64_t Key;
    int64_t Size;

    XImageMipInfo()
    {
        Width = 0;
        Height = 0;
        Format = "";
        Key = 0;
        Size = 0;
    }

    XImageMipInfo(uint32_t SizeWidth, uint32_t SizeHeight, std::string FormatStr, uint64_t KeyVal, uint64_t SizeVal)
    {
        Width = SizeWidth;
        Height = SizeHeight;
        Format = FormatStr;
        Key = KeyVal;
        Size = SizeVal;
    }
};

uint16_t GetFormatFromString(const std::string& Format);

bool XPAKSupport::ParseXPAK(const std::string& FilePath)
{
    // Index list of an XPAK (images only)
    auto Reader = BinaryReader();
    Reader.Open(FilePath);

    auto Header = Reader.Read<BO3XPakHeader>();

    // If MW4 we need to skip the new bytes
    if (Header.Version == 0xD)
    {
        Reader.SetPosition(0);
        uint64_t Result;
        Reader.Read((uint8_t*)&Header, 24, Result);
        Reader.Advance(288);
        Reader.Read((uint8_t*)&Header + 24, 96, Result);
    }

    WraithNameIndex XPAKNames;

    if (Header.Magic == 0x4950414b)
    {
        // Check if this is just an index package
        if (Header.HashCount <= 0)
        {
            return true;
        }

        // The hash table lies in the file: a count beyond it is a corrupt header
        if (Header.HashCount > Reader.GetLength() / sizeof(BO3XPakHashEntry))
        {
            return false;
        }

        if (Header.Version == 11)
        {
            XPAKNames.LoadIndex((std::filesystem::path(FileSystems::GetApplicationPath()) / "package_index\\bo4_ximage.wni").string());
        }

        Reader.SetPosition(Header.HashOffset);

        std::set<uint64_t> IncludedHashes;

        {
            uint64_t HashResult = 0;
            auto Buffer = Reader.Read(Header.HashCount * sizeof(BO3XPakHashEntry), HashResult);
            auto HashData = MemoryReader(Buffer, HashResult);

            for (uint64_t i = 0; i < Header.HashCount; i++)
            {
                IncludedHashes.insert(HashData.Read<BO3XPakHashEntry>().Key);
            }
        }

        Reader.SetPosition(Header.IndexOffset);

        int64_t Width = 0;
        int64_t Height = 0;
        int64_t Size = 0;
        std::string Format = "";
        std::string Name = "";
        std::string Type = "";

        std::unordered_map<std::string, XImageMipInfo> ImageMipMaps;

        for (uint64_t i = 0; i < Header.IndexCount; i++)
        {
            Width = 0;
            Height = 0;
            Size = -1;
            Height = 0;
            Format = "";
            Name = "";
            Type = "";

            // an entry (hash, size, properties) past the end of the file: the index ends here
            if (Reader.GetPosition() + 16 > Reader.GetLength())
            {
                break;
            }

            auto Hash = Reader.Read<uint64_t>();
            auto Properties = Reader.Read<uint64_t>();

            if (Properties > Reader.GetLength() - Reader.GetPosition())
            {
                break;
            }

            if (IncludedHashes.find(Hash) == IncludedHashes.end())
            {
                Reader.Advance(Properties);
                continue;
            }

            uint64_t ReadSize = 0;
            auto PropertiesBuffer = Reader.Read(Properties, ReadSize);

            if (PropertiesBuffer != nullptr)
            {
                auto ResultBuffer = Strings::SplitString(std::string((char*)PropertiesBuffer, (char*)PropertiesBuffer + ReadSize), '\n');

                // The number after "key: ", Value left as it was when there is none
                const auto ReadNumber = [](const std::string& Text, int64_t& Value)
                {
                    const auto Start = Text.find_first_not_of(" \t");
                    if (Start != std::string::npos)
                        std::from_chars(Text.data() + Start, Text.data() + Text.size(), Value);
                };

                for (auto& KeyValue : ResultBuffer)
                {
                    if (_strnicmp(KeyValue.c_str(), "width:", 6) == 0)
                    {
                        // Notice: Width is actually height
                        ReadNumber(KeyValue.substr(7), Height);
                    }
                    else if (_strnicmp(KeyValue.c_str(), "height:", 7) == 0)
                    {
                        // Notice: Height is actually width
                        ReadNumber(KeyValue.substr(8), Width);
                    }
                    else if (_strnicmp(KeyValue.c_str(), "size0:", 6) == 0)
                    {
                        ReadNumber(KeyValue.substr(7), Size);
                    }
                    else if (_strnicmp(KeyValue.c_str(), "name:", 5) == 0)
                    {
                        Name = KeyValue.substr(6);

                        if (Header.Version == 11)
                        {
                            // Convert the hex value and mask to 60bit
                            int64_t Result = std::strtoull(Name.c_str(), 0, 16) & 0xFFFFFFFFFFFFFFF;

                            if (Result != 0)
                            {
                                Name = std::format("ximage_{:x}", Result);

                                if (XPAKNames.NameDatabase.find(Result) != XPAKNames.NameDatabase.end())
                                    Name = XPAKNames.NameDatabase[Result];
                            }

                        }
                    }
                    else if (_strnicmp(KeyValue.c_str(), "format:", 7) == 0)
                    {
                        Format = KeyValue.substr(8);
                    }
                    else if (_strnicmp(KeyValue.c_str(), "type:", 5) == 0)
                    {
                        Type = KeyValue.substr(6);
                    }
                }

                if (Type == "image")
                {
                    if (ImageMipMaps[Name].Width < (uint32_t)Width)
                    {
                        ImageMipMaps[Name] = XImageMipInfo(Width, Height, Format, Hash, Size);
                    }
                }

                delete[] PropertiesBuffer;
            }
        }

        for (auto& KeyValue : ImageMipMaps)
        {
            auto LoadedImage = new CoDImage_t();
            LoadedImage->AssetName = KeyValue.first;
            LoadedImage->AssetPointer = KeyValue.second.Key;
            LoadedImage->AssetSize = KeyValue.second.Size;
            LoadedImage->Width = (uint16_t)KeyValue.second.Width;
            LoadedImage->Height = (uint16_t)KeyValue.second.Height;
            LoadedImage->Format = GetFormatFromString(KeyValue.second.Format);
            LoadedImage->AssetStatus = WraithAssetStatus::Loaded;
            LoadedImage->IsFileEntry = true;

            CoDAssets::GameAssets->LoadedAssets.push_back(LoadedImage);
        }
    }
    else
    {
        return false;
    }

    // The game whose package this is reads its images (ICoDGame.h)
    if (const ICoDGame* Owner = LiveGames::OwnerOf(PackageMark::Container::Xpak, Header.Version))
        CoDAssets::Game = Owner;

    return true;
}

std::unique_ptr<XImageDDS> XPAKSupport::ReadImageFile(const CoDImage_t* Image)
{
    auto ImageFormat = Image->Format;

    // Calculate proper image format (Convert signed to unsigned)
    switch (Image->Format)
    {
    // Fix invalid BC1_SRGB images, swap to BC1_UNORM
    case 72: ImageFormat = 71; break;
    // Fix invalid BC2_SRGB images, swap to BC2_UNORM
    case 75: ImageFormat = 74; break;
    // Fix invalid BC3_SRGB images, swap to BC3_UNORM
    case 78: ImageFormat = 77; break;
    // Fix invalid BC7_SRGB images, swap to BC7_UNORM
    case 99: ImageFormat = 98; break;
    }

    uint32_t ResultSize = 0;
    // AssetPointer = image key
    auto ImageData = CoDAssets::GamePackageCache->ExtractPackageObject(Image->AssetPointer, Image->AssetSize, ResultSize);

    if (ImageData != nullptr)
    {
        auto Result = CoDRawImageTranslator::TranslateBC(ImageData, ResultSize, Image->Width, Image->Height, (uint8_t)ImageFormat);

        if (Result != nullptr && (SettingsManager::GetSetting("patchnormals", "true") == "true"))
        {
            if (Image->AssetName.ends_with("_n") || Image->AssetName.ends_with("_nml") || Image->AssetName.ends_with("_n2") || Image->AssetName.ends_with("_n3"))
            {
                Result->ImagePatchType = ImagePatch::Normal_Expand;
            }
        }

        return Result;
    }

    return nullptr;
}

uint16_t GetFormatFromString(const std::string& Format)
{
    if (Format == "BC1" || Format == "BC1_SRGB" || Format == "33" || Format == "34" || Format == "35")
    {
        return 71;
    }
    else if (Format == "BC2" || Format == "BC2_SRGB")
    {
        return 74;
    }
    else if (Format == "BC3" || Format == "BC3_SRGB" || Format == "38")
    {
        return 77;
    }
    else if (Format == "BC4" || Format == "BC4_SNORM" || Format == "39")
    {
        return 80;
    }
    else if (Format == "BC5")
    {
        return 83;
    }
    else if (Format == "BC5_SNORM")
    {
        return 84;
    }
    else if (Format == "BC6_UH")
    {
        return 95;
    }
    else if (Format == "BC6_SH")
    {
        return 96;
    }
    else if (Format == "R9G9B9E5")
    {
        return 67;
    }
    else if (Format == "R16G16B16A16F")
    {
        return 10;
    }
    else if (Format == "R8G8B8A8_SRGB" || Format == "A8R8G8B8_SRGB" || Format == "R8G8B8A8" || Format == "7")
    {
        return 28;
    }
    else if (Format == "R8_UN")
    {
        return 61;
    }
    else if (Format == "BC7" || Format == "BC7_SRGB" || Format == "44" || Format == "45")
    {
        return 98;
    }

    return 71;
}