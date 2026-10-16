#pragma once

#include <cstdint>
#include <string>
#include <vector>

// What kind of asset an index entry describes. The viewer previews each kind differently and
// the filter system queries across them.
enum class AssetKind : uint8_t
{
    Image,
    Model,
    Audio,
    RawFile,
    Other,
};

// Metadata for an image entry, read cheaply from the IWi header (no pixel decode). This is what
// the icon-finding filters query: "small 2D image with transparency" = Image + small W/H + alpha.
struct ImageMeta
{
    uint16_t Width = 0;
    uint16_t Height = 0;
    uint8_t Format = 0;    // IWi format byte
    bool HasAlpha = false; // format carries an alpha channel (ARGB / A8 / DXT3 / DXT5)
    bool Cubemap = false;
};

// Metadata for a model entry, read from the .semodel or .cast header (cheap - counts only). Enough to
// filter "static prop" (few/one bone) vs "character" (many bones), or by mesh/material count.
struct ModelMeta
{
    uint32_t Bones = 0;
    uint32_t Meshes = 0;
    uint32_t Materials = 0;
};

struct AssetEntry
{
    AssetKind Kind = AssetKind::Other;
    std::string Name;      // asset name (e.g. "images/hud_compass") - no extension
    std::string Source;    // where it was found (e.g. "iw_23.iwd" or a folder)
    std::string Path;      // full on-disk path, set for Model entries so previews can open them
    uint64_t Size = 0;     // uncompressed size in its package / file size
    ImageMeta Image;       // valid when Kind == Image
    ModelMeta Model;       // valid when Kind == Model
};

// A searchable catalogue of assets, built by reusing the existing package readers. Persists to
// JSON so a big catalogue is scanned once and reloaded instantly.
class AssetIndex
{
public:
    std::vector<AssetEntry> Entries;

    // Scans every .iwd under InstallDir for images (IWi headers) and audio (.wav), appending
    // entries. Returns the number added. Reuses MiniZ exactly as IWDCache does.
    size_t IndexIWDPackages(const std::string& InstallDir);

    // Scans Dir recursively for .semodel and .cast model files, reading each header's bone/mesh/material counts.
    // Returns the number added.
    size_t IndexModelDir(const std::string& Dir);

    // JSON persistence.
    bool Save(const std::string& Path) const;
    bool Load(const std::string& Path);

    size_t Count() const { return Entries.size(); }

    // True when an IWi format byte carries a real alpha channel.
    static bool FormatHasAlpha(uint8_t IwiFormat);
    // Human name for an IWi format byte (for reports/UI).
    static const char* FormatName(uint8_t IwiFormat);
    // Kind <-> string for JSON.
    static const char* KindName(AssetKind Kind);
    static AssetKind KindFromName(const std::string& Name);
};
