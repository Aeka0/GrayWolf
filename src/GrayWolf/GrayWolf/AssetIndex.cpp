#include "stdafx.h"

#include "AssetIndex.h"
#include "ModelPreview.h" // ReadSEModelHeaderCounts
#include "CastReader.h"   // CastFile::ModelCounts

#include <miniz/miniz.h>
#include "Strings.h"
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <cstring>

bool AssetIndex::FormatHasAlpha(uint8_t IwiFormat)
{
    // From CoDIWITranslator's format switch: ARGB (0x01), A8 (0x04/0x05), DXT3/BC2 (0x0C),
    // DXT5/BC3 (0x0D) carry alpha; RGB (0x02), DXT1/BC1 (0x0B), BC5 (0x0E), D16 (0x03) do not.
    switch (IwiFormat)
    {
    case 0x01: case 0x04: case 0x05: case 0x0C: case 0x0D:
        return true;
    default:
        return false;
    }
}

const char* AssetIndex::FormatName(uint8_t IwiFormat)
{
    switch (IwiFormat)
    {
    case 0x01: return "ARGB32";
    case 0x02: return "RGB24";
    case 0x03: return "D16";
    case 0x04: case 0x05: return "A8";
    case 0x0B: return "DXT1";
    case 0x0C: return "DXT3";
    case 0x0D: return "DXT5";
    case 0x0E: return "BC5";
    default:   return "unknown";
    }
}

const char* AssetIndex::KindName(AssetKind Kind)
{
    switch (Kind)
    {
    case AssetKind::Image:   return "image";
    case AssetKind::Model:   return "model";
    case AssetKind::Audio:   return "audio";
    case AssetKind::RawFile: return "rawfile";
    default:                 return "other";
    }
}

AssetKind AssetIndex::KindFromName(const std::string& Name)
{
    if (Name == "image")   return AssetKind::Image;
    if (Name == "model")   return AssetKind::Model;
    if (Name == "audio")   return AssetKind::Audio;
    if (Name == "rawfile") return AssetKind::RawFile;
    return AssetKind::Other;
}

// Reads the 10-byte IWi header from a decompressed entry buffer into ImageMeta. Returns false if
// it is not a valid IWi.
static bool ReadIwiMeta(const uint8_t* Data, size_t Length, ImageMeta& Out)
{
    // IWIHeader: Magic[3]="IWi", Version(1). IWIInfo: Format(1)@4, Flags(1)@5, W(u16)@6, H(u16)@8.
    if (Length < 10 || Data[0] != 0x49 || Data[1] != 0x57 || Data[2] != 0x69)
        return false;

    Out.Format = Data[4];
    const uint8_t Flags = Data[5];
    Out.Width = (uint16_t)(Data[6] | (Data[7] << 8));
    Out.Height = (uint16_t)(Data[8] | (Data[9] << 8));
    Out.HasAlpha = AssetIndex::FormatHasAlpha(Out.Format);
    Out.Cubemap = (Flags & 0x4) != 0;
    return true;
}

size_t AssetIndex::IndexIWDPackages(const std::string& InstallDir)
{
    namespace fs = std::filesystem;
    std::error_code Ec;
    const size_t Before = Entries.size();

    // Collect every .iwd under the install (main/ holds them, but scan broadly).
    std::vector<std::string> Packages;
    for (auto It = fs::recursive_directory_iterator(InstallDir, Ec);
         It != fs::recursive_directory_iterator(); It.increment(Ec))
    {
        if (Ec) break;
        if (!It->is_regular_file(Ec)) continue;
        auto Path = It->path();
        auto Ext = Path.extension().string();
        for (auto& C : Ext) C = (char)tolower((unsigned char)C);
        if (Ext == ".iwd")
            Packages.push_back(Path.string());
    }

    std::vector<uint8_t> Buffer;

    for (const auto& PackagePath : Packages)
    {
        const std::string PackageName = fs::path(PackagePath).filename().string();

        mz_zip_archive Zip;
        std::memset(&Zip, 0, sizeof(Zip));
        if (!mz_zip_reader_init_file(&Zip, PackagePath.c_str(), 0))
            continue;

        const uint32_t FileCount = (uint32_t)mz_zip_reader_get_num_files(&Zip);
        for (uint32_t i = 0; i < FileCount; i++)
        {
            mz_zip_archive_file_stat Info;
            if (!mz_zip_reader_file_stat(&Zip, i, &Info))
                continue;
            if (Info.m_is_directory)
                continue;

            std::string EntryName = Info.m_filename;
            std::string Lower = EntryName;
            for (auto& C : Lower) C = (char)tolower((unsigned char)C);

            if (Lower.ends_with(".iwi"))
            {
                // Decompress the entry (IWi in IWD are usually stored, so this is a copy) and
                // read only its header.
                if (Info.m_uncomp_size < 10)
                    continue;
                Buffer.resize((size_t)Info.m_uncomp_size);
                if (!mz_zip_reader_extract_to_mem(&Zip, i, Buffer.data(), Buffer.size(), 0))
                    continue;

                ImageMeta Meta;
                if (!ReadIwiMeta(Buffer.data(), Buffer.size(), Meta))
                    continue;

                AssetEntry Entry;
                Entry.Kind = AssetKind::Image;
                // Drop the .iwi extension for a clean asset name.
                Entry.Name = EntryName.substr(0, EntryName.size() - 4);
                Entry.Source = PackageName;
                Entry.Size = Info.m_uncomp_size;
                Entry.Image = Meta;
                Entries.push_back(std::move(Entry));
            }
            else if (Lower.ends_with(".wav"))
            {
                // Audio catalogued by name/size (cheap, no extraction); richer meta later.
                AssetEntry Entry;
                Entry.Kind = AssetKind::Audio;
                Entry.Name = EntryName.substr(0, EntryName.size() - 4);
                Entry.Source = PackageName;
                Entry.Size = Info.m_uncomp_size;
                Entries.push_back(std::move(Entry));
            }
        }

        mz_zip_reader_end(&Zip);
    }

    return Entries.size() - Before;
}

size_t AssetIndex::IndexModelDir(const std::string& Dir)
{
    namespace fs = std::filesystem;
    std::error_code Ec;
    const size_t Before = Entries.size();

    for (auto It = fs::recursive_directory_iterator(Dir, Ec);
         It != fs::recursive_directory_iterator(); It.increment(Ec))
    {
        if (Ec) break;
        if (!It->is_regular_file(Ec)) continue;

        auto Path = It->path();
        auto Ext = Path.extension().string();
        for (auto& C : Ext) C = (char)tolower((unsigned char)C);
        if (Ext != ".semodel" && Ext != ".cast") continue;

        // a .cast that holds an anim is no model: ModelCounts fails on it
        ModelMeta Meta;
        const bool Read = (Ext == ".cast") ? CastFile::ModelCounts(Path.string(), Meta.Bones, Meta.Meshes, Meta.Materials)
                                           : ReadSEModelHeaderCounts(Path.string(), Meta.Bones, Meta.Meshes, Meta.Materials);
        if (!Read) continue;

        AssetEntry Entry;
        Entry.Kind = AssetKind::Model;
        Entry.Name = Path.stem().string();
        Entry.Source = Path.parent_path().filename().string();
        Entry.Path = Path.string();
        Entry.Size = (uint64_t)fs::file_size(Path, Ec);
        Entry.Model = Meta;
        Entries.push_back(std::move(Entry));
    }

    return Entries.size() - Before;
}

bool AssetIndex::Save(const std::string& Path) const
{
    nlohmann::json Root = nlohmann::json::array();
    for (const auto& E : Entries)
    {
        nlohmann::json J;
        J["kind"] = KindName(E.Kind);
        J["name"] = E.Name;
        J["source"] = E.Source;
        J["size"] = E.Size;
        if (E.Kind == AssetKind::Image)
        {
            J["w"] = E.Image.Width;
            J["h"] = E.Image.Height;
            J["fmt"] = E.Image.Format;
            J["fmtName"] = FormatName(E.Image.Format);
            J["alpha"] = E.Image.HasAlpha;
            J["cube"] = E.Image.Cubemap;
        }
        else if (E.Kind == AssetKind::Model)
        {
            J["bones"] = E.Model.Bones;
            J["meshes"] = E.Model.Meshes;
            J["materials"] = E.Model.Materials;
            if (!E.Path.empty()) J["path"] = E.Path;
        }
        Root.push_back(std::move(J));
    }

    std::ofstream Out(Path, std::ios::binary | std::ios::trunc);
    if (!Out) return false;
    Out << Root.dump();
    return (bool)Out;
}

bool AssetIndex::Load(const std::string& Path)
{
    std::ifstream In(Path, std::ios::binary);
    if (!In) return false;

    nlohmann::json Root;
    try { In >> Root; }
    catch (...) { return false; }
    if (!Root.is_array()) return false;

    Entries.clear();
    Entries.reserve(Root.size());
    for (const auto& J : Root)
    {
        AssetEntry E;
        E.Kind = KindFromName(J.value("kind", std::string("other")));
        E.Name = J.value("name", std::string());
        E.Source = J.value("source", std::string());
        E.Size = J.value("size", (uint64_t)0);
        if (E.Kind == AssetKind::Image)
        {
            E.Image.Width = (uint16_t)J.value("w", 0);
            E.Image.Height = (uint16_t)J.value("h", 0);
            E.Image.Format = (uint8_t)J.value("fmt", 0);
            E.Image.HasAlpha = J.value("alpha", false);
            E.Image.Cubemap = J.value("cube", false);
        }
        else if (E.Kind == AssetKind::Model)
        {
            E.Model.Bones = J.value("bones", 0u);
            E.Model.Meshes = J.value("meshes", 0u);
            E.Model.Materials = J.value("materials", 0u);
            E.Path = J.value("path", std::string());
        }
        Entries.push_back(std::move(E));
    }
    return true;
}
