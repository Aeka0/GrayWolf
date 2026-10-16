#include "stdafx.h"

#include "ZoneSource.h"

#include "FastFile.h"
#include "OatInProcess.h"
#include "FileSystems.h"
#include "Games/GameModule.h"

#include <algorithm>
#include <map>
#include <set>
#include <utility>

namespace
{
    // IW3 through T6: OpenAssetTools loads the zone and knows its asset list and its writers.
    class OatZoneSource final : public IZoneSource
    {
    public:
        explicit OatZoneSource(OatZoneListing Listing)
            : m_listing(std::move(Listing))
        {
            m_assets.reserve(m_listing.Assets.size());
            for (const auto& Entry : m_listing.Assets)
                m_assets.push_back({ Entry.TypeId, Entry.TypeName, Entry.Name, Entry.IsReference });
        }

        [[nodiscard]] const std::string& Game() const override { return m_listing.Game; }
        [[nodiscard]] const std::string& ZoneName() const override { return m_listing.Zone; }
        [[nodiscard]] ZoneSourceKind Kind() const override { return ZoneSourceKind::Disk; }
        [[nodiscard]] const std::vector<ZoneSourceAsset>& Assets() const override { return m_assets; }
        [[nodiscard]] uint64_t ZoneBytes() const override { return 0; }
        [[nodiscard]] std::vector<uint32_t> ExtractableTypes() const override { return m_listing.ExtractableTypes; }

    private:
        OatZoneListing m_listing;
        std::vector<ZoneSourceAsset> m_assets;
    };

    // A container we can open but whose asset list is not modelled yet. Saying so explicitly beats
    // reporting an empty zone, which reads like "nothing in here".
    class ContainerOnlyZoneSource final : public IZoneSource
    {
    public:
        ContainerOnlyZoneSource(std::string GameName, std::string ZoneName, uint64_t Bytes)
            : m_game(std::move(GameName)), m_zone(std::move(ZoneName)), m_bytes(Bytes)
        {
        }

        [[nodiscard]] const std::string& Game() const override { return m_game; }
        [[nodiscard]] const std::string& ZoneName() const override { return m_zone; }
        [[nodiscard]] ZoneSourceKind Kind() const override { return ZoneSourceKind::DiskContainerOnly; }
        [[nodiscard]] const std::vector<ZoneSourceAsset>& Assets() const override { return m_empty; }
        [[nodiscard]] uint64_t ZoneBytes() const override { return m_bytes; }
        [[nodiscard]] std::vector<uint32_t> ExtractableTypes() const override { return {}; }

    private:
        std::string m_game;
        std::string m_zone;
        uint64_t m_bytes = 0;
        std::vector<ZoneSourceAsset> m_empty;
    };
} // namespace

namespace
{
    std::map<std::string, ZoneSourceFactory>& ZoneSourceFactories()
    {
        static std::map<std::string, ZoneSourceFactory> Factories;
        return Factories;
    }
}

ZoneSourceRegistration::ZoneSourceRegistration(const char* Engine, ZoneSourceFactory Factory)
{
    ZoneSourceFactories()[Engine] = Factory;
}

std::unique_ptr<IZoneSource> OpenZoneSource(const std::string& FastFilePath, std::string& Error)
{
    // Ask the reader that models the most first: OAT gives a real asset list, so a zone it
    // understands should never fall through to the container-only path below.
    OatZoneListing Listing;
    std::string OatError;
    if (OatDescribeZone(FastFilePath, Listing, OatError))
        return std::make_unique<OatZoneSource>(std::move(Listing));

    // Otherwise the container may still be readable even though nothing models its contents yet.
    const auto Info = FastFile::Identify(FastFilePath);
    if (Info.Format != FastFileFormat::Unknown)
    {
        // The title comes from the module table rather than from whichever reader happens to know
        // a name for it, so "which game is this" has one answer.
        const auto* Module = FindGameModule(Info.Magic, Info.Version, FastFilePath);
        const auto Layout = FastFile::LayoutFor(Info);

        std::vector<uint8_t> Zone;
        std::string DecompressError;
        if (FastFile::Decompress(FastFilePath, Zone, DecompressError))
        {
            const std::string Title = Module            ? Module->Title
                                    : !Layout.Title.empty() ? Layout.Title
                                                            : Info.Magic;
            const auto Name = FileSystems::GetFileNameWithoutExtension(FastFilePath);

            if (Module != nullptr)
            {
                auto It = ZoneSourceFactories().find(Module->Engine);
                if (It != ZoneSourceFactories().end())
                {
                    auto Source = It->second(Title, Name, Zone, Error);
                    return Source;
                }
            }

            return std::make_unique<ContainerOnlyZoneSource>(Title, Name, (uint64_t)Zone.size());
        }

        Error = DecompressError;
        return nullptr;
    }

    Error = OatError.empty() ? "no reader understands this container" : OatError;
    return nullptr;
}

std::string ZoneSourceCoverageReport(const IZoneSource& Source)
{
    std::map<uint32_t, std::pair<std::string, uint32_t>> PerType;
    uint32_t Total = 0;

    for (const auto& Asset : Source.Assets())
    {
        // A reference names an asset defined in another zone, so it has no content to extract here.
        if (Asset.IsReference)
            continue;

        auto& Entry = PerType[Asset.TypeId];
        if (Entry.first.empty())
            Entry.first = Asset.TypeName;
        ++Entry.second;
        ++Total;
    }

    const auto Extractable = Source.ExtractableTypes();
    const std::set<uint32_t> Covered(Extractable.begin(), Extractable.end());

    std::vector<std::pair<uint32_t, std::pair<std::string, uint32_t>>> Sorted(PerType.begin(), PerType.end());
    std::sort(Sorted.begin(), Sorted.end(), [](const auto& A, const auto& B)
    {
        return A.second.second > B.second.second;
    });

    size_t CoveredTypes = 0;
    for (const auto& Entry : Sorted)
    {
        if (Covered.count(Entry.first) > 0)
            ++CoveredTypes;
    }

    std::string Report = "zone=" + Source.ZoneName() + "\n";
    Report += "game=" + Source.Game() + "\n";
    Report += "total_assets=" + std::to_string(Total) + "\n";
    Report += "covered=" + std::to_string(CoveredTypes) + "/" + std::to_string(Sorted.size()) + "\n";

    // Same shape as OAT's own report, so a reader of one reads either.
    for (const auto& Entry : Sorted)
    {
        const std::string Name = Entry.second.first.empty() ? std::to_string(Entry.first) : Entry.second.first;
        const char* Via = Covered.count(Entry.first) > 0 ? "ours" : "none";
        Report += "type=" + Name + " count=" + std::to_string(Entry.second.second) + " via=" + Via + "\n";
    }

    for (const auto& Entry : Sorted)
    {
        if (Covered.count(Entry.first) > 0)
            continue;

        const std::string Name = Entry.second.first.empty() ? std::to_string(Entry.first) : Entry.second.first;
        Report += "uncovered=" + Name + " count=" + std::to_string(Entry.second.second) + "\n";
    }

    return Report + Source.ExtraReport();
}
