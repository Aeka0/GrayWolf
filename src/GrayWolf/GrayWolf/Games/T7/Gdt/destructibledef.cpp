#include "stdafx.h"

// BO3 destructibledef (2) -> a destructibledef entry and one destructiblepiece entry per piece. HydraX
// (github.com/Scobalula/HydraX, GPL-3.0) left the type unfinished (its piece table, 7 stages); the
// header and the piece's own fields were read off mp_sector's veh_t7_civ_citycar_destructible against
// the Mod Tools' veh_t7_civ_city_car.gdt. Header {name 0, model 8, pristine model 0x10, piece count
// 0x18, pieces 0x20, clientOnly 0x28, syncBaseHealthWithEntity 0x2c}; a piece's layout is the type's
// destructibledef_fields.jsonc: a key holding '#' goes to the def entry with the piece's index, a
// flags<stage> number is split into the bits type.jsonc's "flags" names, the rest is the piece's.
// The zone keeps the pieces inline and unnamed: <def>_piece<index>. A piece's damage sound is written
// back as the GDT had it: the Linker fills an empty one with dst_<def>_d<index>, "NULL" with nothing.

#include "Gdt.h"

#include <format>
#include <utility>

namespace
{
    std::string Replace(std::string S, const std::string& By)
    {
        const size_t At = S.find('#');
        return At == std::string::npos ? S : S.replace(At, 1, By);
    }

    void Constants(const nlohmann::ordered_json& Settings, const char* Key, Gdt::Block& B)
    {
        if (Settings.contains(Key))
            for (const auto& [K, V] : Settings.at(Key).items()) B.Keys.emplace_back(K, V.get<std::string>());
    }

    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out,
                 Gdt::Stats& S)
    {
        const auto& Settings = T.Settings();
        const int64_t Size = std::stoll(Settings.value("piece_size", std::string("0x3c8")), nullptr, 16);
        const int Slots = Settings.value("slots", 64);
        struct Bit
        {
            std::string Key;
            int Index = 0;
            bool Staged = false;
        };
        std::vector<Bit> Bits;
        if (Settings.contains("flags"))
            for (const auto& [K, V] : Settings.at("flags").items())
                Bits.push_back(V.is_object() ? Bit{ K, V.value("bit", 0), V.value("staged", false) } : Bit{ K, V.get<int>(), false });

        Gdt::Block Def{ Name, "destructibledef.gdf", "", {} };
        Def.Keys.emplace_back("model", M.AssetField(Pos + 8));
        Def.Keys.emplace_back("radiantModel", M.AssetField(Pos + 0x10));
        Def.Keys.emplace_back("clientOnly", std::to_string(M.At<int32_t>(Pos + 0x28) != 0));
        Def.Keys.emplace_back("syncBaseHealthWithEntity", std::to_string(M.At<int32_t>(Pos + 0x2c) != 0));
        Constants(Settings, "def_constants", Def);

        const int64_t Pieces = M.Follow(Pos + 0x20);
        const int Count = Pieces < 0 ? 0 : (int)std::min<uint32_t>(M.At<uint32_t>(Pos + 0x18), (uint32_t)Slots);
        for (int I = 0; I < Slots; I++)
        {
            const std::string Index = std::to_string(I);
            if (I >= Count)
            {
                if (Settings.contains("slot_defaults"))
                    for (const auto& [K, V] : Settings.at("slot_defaults").items()) Def.Keys.emplace_back(Replace(K, Index), V.get<std::string>());
                continue;
            }
            Gdt::Block Piece{ std::format("{}_piece{}", Name, I), "destructiblepiece.gdf", "", {} };
            Def.Keys.emplace_back("piece" + Index, Piece.Name);
            Def.Keys.emplace_back("pieceLabel" + Index, "");
            for (auto& [K, V] : T.Fields(M, Pieces + Size * I, S))
            {
                if (K == "damageSound#" && Settings.contains("damage_sound_default"))
                {
                    // the Linker names an empty one dst_<def>_d<index>, "NULL" is none
                    std::string Default = Settings.at("damage_sound_default").get<std::string>();
                    for (const auto& [From, To] : { std::pair<std::string, std::string>{ "{def}", Name }, { "{index}", Index } })
                        if (const size_t At = Default.find(From); At != std::string::npos) Default.replace(At, From.size(), To);
                    Def.Keys.emplace_back(Replace(K, Index), V.empty() ? "NULL" : V == Default ? "" : V);
                }
                else if (K.find('#') != std::string::npos) Def.Keys.emplace_back(Replace(K, Index), std::move(V));
                else if (K.rfind("flags", 0) == 0)
                {
                    const std::string Stage = K.substr(5);
                    const uint32_t Flags = (uint32_t)std::strtoul(V.c_str(), nullptr, 10);
                    for (const Bit& B : Bits)
                        if (!B.Staged || !Stage.empty()) Piece.Keys.emplace_back(B.Key + Stage, (Flags >> B.Index) & 1 ? "1" : "0");
                }
                else Piece.Keys.emplace_back(K, std::move(V));
            }
            Constants(Settings, "piece_constants", Piece);
            Out.push_back(std::move(Piece));
        }
        S.Counters["destructibledef_pieces"] += Count;
        Out.push_back(std::move(Def));
        return true;
    }
}

GDT_CONVERTER(t7_destructibledef, Convert);
