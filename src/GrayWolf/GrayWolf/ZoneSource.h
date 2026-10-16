#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// One way to talk about "a zone on disk", whichever engine wrote it.
//
// Everything that reads a zone should end up behind this interface, so callers (coverage,
// extraction, the GUI) stop branching per game.
//
// Deliberately narrow: opening a container and listing what is inside is the part every engine has
// in common. Typed access stays with the per-game code behind it.

// What a zone says it holds. TypeName is the engine's own name for the type ("xmodel", "clipmap"),
// empty when the engine has no name for that id.
struct ZoneSourceAsset
{
    uint32_t    TypeId = 0;
    std::string TypeName;
    std::string Name;
    // A reference is an asset defined in another zone; it carries no content here, so coverage and
    // extraction both have to skip it or they count the same asset many times over.
    bool        IsReference = false;
};

// How a zone was reached, which is what the disk-first goal actually cares about.
enum class ZoneSourceKind
{
    // Read straight from the fastfile, no game running. The goal for every title.
    Disk,
    // The container decompresses but its asset list is not modelled yet.
    DiskContainerOnly,
};

class IZoneSource
{
public:
    virtual ~IZoneSource() = default;

    IZoneSource(const IZoneSource&) = delete;
    IZoneSource& operator=(const IZoneSource&) = delete;

    // Engine short name as the reader knows it: "IW3".."IW6", "T4".."T8".
    [[nodiscard]] virtual const std::string& Game() const = 0;
    [[nodiscard]] virtual const std::string& ZoneName() const = 0;
    [[nodiscard]] virtual ZoneSourceKind Kind() const = 0;

    // Empty when Kind() is DiskContainerOnly: the bytes are readable, the list is not modelled.
    [[nodiscard]] virtual const std::vector<ZoneSourceAsset>& Assets() const = 0;

    // Decompressed size in bytes, 0 when the reader never materialises the zone.
    [[nodiscard]] virtual uint64_t ZoneBytes() const = 0;

    // Which asset type ids this source can actually write out, asked of the writers themselves
    // rather than kept in a list beside them.
    [[nodiscard]] virtual std::vector<uint32_t> ExtractableTypes() const = 0;

    // Anything a particular reader knows that the shared report has no field for. Empty by
    // default; Ghosts uses it to say how far walking the assets' own bytes got.
    [[nodiscard]] virtual std::string ExtraReport() const { return {}; }

protected:
    IZoneSource() = default;
};

// A reader for one engine's decompressed zones (Games/<game>/ZoneSource.cpp registers it with a
// static ZoneSourceRegistration). Returns nullptr with Error set when the zone does not read.
using ZoneSourceFactory = std::unique_ptr<IZoneSource> (*)(const std::string& Title, const std::string& ZoneName,
                                                           std::vector<uint8_t>& Zone, std::string& Error);
struct ZoneSourceRegistration
{
    ZoneSourceRegistration(const char* Engine, ZoneSourceFactory Factory);
};

// Opens whichever reader understands this file. Returns nullptr with a reason when no reader does,
// which is the honest answer for a container we have not modelled yet.
std::unique_ptr<IZoneSource> OpenZoneSource(const std::string& FastFilePath, std::string& Error);

// The same per-type coverage report OAT produces, built from any source, for the titles OAT cannot
// load at all.
std::string ZoneSourceCoverageReport(const IZoneSource& Source);
