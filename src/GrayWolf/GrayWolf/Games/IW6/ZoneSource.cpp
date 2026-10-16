#include "stdafx.h"

#include "ZoneSource.h"
#include "ExportWriters.h"
#include "Iw6AssetList.h"

#include <string>
#include <utility>
#include <vector>

namespace
{
    // Ghosts. Its asset list is read straight from the decompressed zone; ghosts-disk-export
    // extracts xanim (2), xmodel (4), image (13), sound (14) and loaded_sound (18).
    class GhostsZoneSource final : public IZoneSource
    {
    public:
        GhostsZoneSource(std::string GameName, std::string ZoneName, uint64_t Bytes,
                         const Iw6::AssetList& List, std::string Walk)
            : m_game(std::move(GameName)), m_zone(std::move(ZoneName)), m_bytes(Bytes),
              m_walk(std::move(Walk))
        {
            m_assets.reserve(List.Assets.size());
            for (const auto& Entry : List.Assets)
            {
                const auto* Name = Iw6::AssetTypeName(Entry.Type);
                m_assets.push_back({ Entry.Type, Name ? Name : "", std::string(), Entry.IsReference });
            }
        }

        [[nodiscard]] const std::string& Game() const override { return m_game; }
        [[nodiscard]] const std::string& ZoneName() const override { return m_zone; }
        [[nodiscard]] ZoneSourceKind Kind() const override { return ZoneSourceKind::Disk; }
        [[nodiscard]] const std::vector<ZoneSourceAsset>& Assets() const override { return m_assets; }
        [[nodiscard]] uint64_t ZoneBytes() const override { return m_bytes; }
        // xanim, xmodel, image, sound, loaded_sound, rawfile, stringtable, luafile
        [[nodiscard]] std::vector<uint32_t> ExtractableTypes() const override { return { 2, 4, 13, 14, 18, 44, 46, 54 }; }
        [[nodiscard]] std::string ExtraReport() const override { return m_walk; }

    private:
        std::string m_game;
        std::string m_zone;
        uint64_t m_bytes = 0;
        std::string m_walk;
        std::vector<ZoneSourceAsset> m_assets;
    };


    std::unique_ptr<IZoneSource> Open(const std::string& Title, const std::string& Name, std::vector<uint8_t>& Zone, std::string& Error)
    {
        Iw6::AssetList List;
        std::string ListError;
        if (Iw6::ReadAssetList(Zone, List, ListError))
        {
            // How much of the zone is understood: the zonekit iw6 walk the disk export itself runs.
            const size_t Total = List.Assets.size();
            std::string ProfileError;
            const auto* Loaded = ExportWriters::ZoneProfile("iw6", ProfileError);
            std::string Walk;
            if (Loaded == nullptr)
            {
                Walk = "walked=0/" + std::to_string(Total) + "\nwalk_end=short\n"
                     + "walk_stopped=" + ProfileError + "\n";
            }
            else
            {
                const ZoneTables::ZoneResult R = ZoneTables::WalkZone(Zone, Loaded->Tables,
                    Loaded->Profile.TypeNames, Loaded->Profile.Rules, true);
                const bool Full = !R.ParseFailed && R.Why == "walked to the end";
                size_t Walked = Total;
                if (!Full)
                {
                    Walked = 0;
                    for (const auto& I : R.Instances)
                        if (I.Parent < 0) ++Walked;
                }
                Walk = "walked=" + std::to_string(Walked) + "/" + std::to_string(Total) + "\n"
                     + "walk_end=" + std::string(Full ? "exact" : "short") + "\n";
                if (!Full)
                    Walk += "walk_stopped=" + R.Why + "\n";
            }

            return std::make_unique<GhostsZoneSource>(Title, Name, (uint64_t)Zone.size(),
                                                     List, Walk);
        }

        Error = ListError;
        return nullptr;
    }

    const ZoneSourceRegistration Registered("IW6", Open);
}
