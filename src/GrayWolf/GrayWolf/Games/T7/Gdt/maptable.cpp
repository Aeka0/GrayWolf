#include "stdafx.h"

// BO3 maptable (90) -> a maptable entry and a maptableentry for each of its maps, as HydraX's
// MapTable.cs (github.com/Scobalula/HydraX, GPL-3.0): table {name 0, count 8, maps 0x10, session
// mode 0x18, campaign mode 0x1c, dlc index 0x20}; a map (0xf8) as read below, its key/values
// (count 0xe4, list 0xe8) written <type>_<name>.

#include "Gdt.h"
#include "Kvp.h"

#include <format>
#include <set>

namespace
{
    constexpr int64_t ImageName = 0xf8;   // an image's name pointer

    bool Convert(const Gdt::Type&, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out,
                 Gdt::Stats& S)
    {
        const auto Str = [&](int64_t At) { return M.StringField(At); };
        const auto Image = [&](int64_t At) { return M.AssetField(At, ImageName); };
        const auto Int = [&](int64_t At) { return std::to_string(M.At<int32_t>(At)); };
        const auto Float = [&](int64_t At) { return Gdt::Number(M.At<float>(At), false); };

        Gdt::Block Table{ Name, "maptable.gdf", "", {} };
        const int64_t Maps = M.Follow(Pos + 0x10);
        const int Count = Maps < 0 ? 0 : (int)std::min<uint32_t>(M.At<uint32_t>(Pos + 8), 1024);
        Table.Keys.emplace_back("mapCount", Int(Pos + 8));
        Table.Keys.emplace_back("sessionMode", Int(Pos + 0x18));
        Table.Keys.emplace_back("campaignMode", Int(Pos + 0x1c));
        Table.Keys.emplace_back("dlcIndex", Int(Pos + 0x20));
        std::set<std::string> Written;
        for (int I = 0; I < Count; I++)
        {
            const int64_t E = Maps + 0xf8 * (int64_t)I;
            Gdt::Block B{ Str(E), "maptableentry.gdf", "", {} };
            auto& K = B.Keys;
            K.emplace_back("size", Int(E + 8));
            K.emplace_back("mapName", Str(E + 0x10));
            K.emplace_back("mapNameCaps", Str(E + 0x18));
            K.emplace_back("rootMapName", Str(E + 0x20));
            K.emplace_back("missionName", Str(E + 0x38));
            K.emplace_back("mapDescription", Str(E + 0x28));
            K.emplace_back("mapDescriptionShort", Str(E + 0x30));
            K.emplace_back("mapLocation", Str(E + 0x40));
            K.emplace_back("mapDateTime", Str(E + 0x48));
            K.emplace_back("presenceString", Str(E + 0x50));
            K.emplace_back("previewImage", Image(E + 0x58));
            K.emplace_back("loadingImage", Str(E + 0x60));
            K.emplace_back("compassMap", Image(E + 0x68));
            K.emplace_back("compassMapCorrupt", Image(E + 0x70));
            K.emplace_back("isSafeHouse", std::to_string(M.At<uint8_t>(E + 0x78)));
            K.emplace_back("isSubLevel", std::to_string(M.At<uint8_t>(E + 0x79)));
            K.emplace_back("Safehouse", Str(E + 0x80));
            K.emplace_back("introMovie", Str(E + 0x88));
            K.emplace_back("outroMovie", Str(E + 0x90));
            K.emplace_back("collectibles", Str(E + 0xa0));
            K.emplace_back("accoladelist", M.AssetField(E + 0xa8));
            K.emplace_back("objectives", M.AssetField(E + 0x98));
            K.emplace_back("mapObjective", Str(E + 0xb0));
            K.emplace_back("engageClose", Float(E + 0xb8));
            K.emplace_back("engageMedium", Float(E + 0xbc));
            K.emplace_back("engageLong", Float(E + 0xc0));
            K.emplace_back("resistanceText", Str(E + 0xc8));
            K.emplace_back("operationsBackground", Image(E + 0xd0));
            K.emplace_back("isFreeRunMap", Int(E + 0xd8));
            K.emplace_back("freerunTrackIndex", Int(E + 0xdc));
            K.emplace_back("mapVersion", Int(E + 0xe0));
            std::vector<std::pair<std::string, std::string>> Kv;
            T7Gdt::KeyValues(M, M.Follow(E + 0xe8), M.At<uint32_t>(E + 0xe4), false, Kv);
            for (auto& [Key, Value] : Kv)
            {
                // HydraX: the Mod Tools name it so though the game types it an xstring
                if (Key.find("bigPreviewImage") != std::string::npos) Key = "streamedimage_bigPreviewImage";
                K.emplace_back(std::move(Key), std::move(Value));
            }
            T7Gdt::Once(K);
            Table.Keys.emplace_back(std::format("map{:02}", I + 1), B.Name);
            if (!T7Gdt::EntryName(B.Name)) S.Counters["maptable_unnamed"]++;
            else if (Written.insert(B.Name).second) Out.push_back(std::move(B));
        }
        S.Counters["maptable_maps"] += Count;
        Out.push_back(std::move(Table));
        return true;
    }
}

GDT_CONVERTER(t7_maptable, Convert);
