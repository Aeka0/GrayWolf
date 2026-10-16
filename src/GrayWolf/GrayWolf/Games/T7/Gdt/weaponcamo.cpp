#include "stdafx.h"

// BO3 weaponcamo (33) -> weaponcamo entries of 75 camos each and the weaponcamotable naming them,
// as HydraX's WeaponCamo.cs (github.com/Scobalula/HydraX, GPL-3.0). Header {name 0, camos 8,
// count 0x10}; a camo (0x10) {materials count 0, materials 8}; a material (0x138) {base materials
// count 0 (u16), base materials 8, flags 0x18, material 0x20, translation 0x28, scale 0x30,
// rotation 0x38, gloss blend 0x3c, normal blend 0x40, detail normal map 0x48, normal height 0x50,
// normal scale 0x54}; a base material (0x10) {material 0, camo mask image 8}.
// Beyond HydraX: the base materials are written 1..10 as the GDF has them, the flags as 1/0 and
// the table's configstringFileType as the Mod Tools write it (WEAPONCAMOTABLE).

#include "Gdt.h"

#include <filesystem>
#include <format>

namespace
{
    constexpr int PerTable = 75, Tables = 10, BaseMaterials = 10;

    std::string Stem(const std::string& N) { return N.empty() ? N : std::filesystem::path(N).stem().string(); }

    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out,
                 Gdt::Stats& S)
    {
        const int64_t Camos = M.Follow(Pos + 8);
        const int Count = Camos < 0 ? 0 : (int)std::min<uint64_t>(M.At<uint64_t>(Pos + 0x10), PerTable * Tables);
        std::vector<std::string> Names;
        for (int I = 0; I * PerTable < Count; I++)
        {
            const int Base = I * PerTable, InTable = std::min(Count - Base, PerTable);
            Gdt::Block B{ Name + (I == 0 ? "_ship" : "_base" + std::to_string(Base + 1)), "weaponcamo.gdf", "", {} };
            B.Keys.emplace_back("configstringFileType", "WEAPONCAMO");
            B.Keys.emplace_back("baseIndex", std::to_string(Base + 1));
            B.Keys.emplace_back("numCamos", std::to_string(InTable));
            for (int J = 0; J < InTable; J++)
            {
                const int64_t Camo = Camos + 0x10 * (int64_t)(Base + J);
                const int64_t Mats = M.Follow(Camo + 8);
                const int MatCount = Mats < 0 ? 0 : (int)std::min<uint64_t>(M.At<uint64_t>(Camo), 8);
                for (int K = 0; K < MatCount; K++)
                {
                    const int64_t Mt = Mats + 0x138 * (int64_t)K;
                    const auto Key = [&](const char* What) { return std::format("material{}_{}_{}", K + 1, J + 1, What); };
                    const auto F = [&](int64_t At) { return Gdt::Number(M.At<float>(Mt + At), false); };
                    const int64_t Bases = M.Follow(Mt + 8);
                    const int BaseCount = Bases < 0 ? 0 : (int)M.At<uint16_t>(Mt);
                    for (int L = 0; L < std::max(BaseCount, BaseMaterials); L++)
                    {
                        const int64_t Bm = L < BaseCount ? Bases + 0x10 * (int64_t)L : -1;
                        B.Keys.emplace_back(std::format("material{}_{}_base_material_{}", K + 1, J + 1, L + 1),
                                            Bm < 0 ? "" : Stem(M.AssetField(Bm)));
                        B.Keys.emplace_back(std::format("material{}_{}_camo_mask_{}", K + 1, J + 1, L + 1),
                                            Bm < 0 ? "" : M.AssetField(Bm + 8, 0xf8));
                    }
                    const uint8_t Flags = M.At<uint8_t>(Mt + 0x18);
                    B.Keys.emplace_back(Key("detail_normal_height"), F(0x50));
                    B.Keys.emplace_back(Key("detail_normal_map"), M.AssetField(Mt + 0x48, 0xf8));
                    B.Keys.emplace_back(Key("detail_normal_scale_x"), F(0x54));
                    B.Keys.emplace_back(Key("detail_normal_scale_y"), F(0x58));
                    B.Keys.emplace_back(Key("gloss_blend"), F(0x3c));
                    B.Keys.emplace_back(Key("normal_amount"), F(0x40));
                    B.Keys.emplace_back(Key("material"), Stem(M.AssetField(Mt + 0x20)));
                    B.Keys.emplace_back(Key("numBaseMaterials"), std::to_string(BaseCount));
                    B.Keys.emplace_back(Key("rotation"), F(0x38));
                    B.Keys.emplace_back(Key("scale_x"), F(0x30));
                    B.Keys.emplace_back(Key("scale_y"), F(0x34));
                    B.Keys.emplace_back(Key("trans_x"), F(0x28));
                    B.Keys.emplace_back(Key("trans_y"), F(0x2c));
                    B.Keys.emplace_back(Key("useGlossMap"), (Flags & 0x8) ? "1" : "0");
                    B.Keys.emplace_back(Key("useNormalMap"), (Flags & 0x2) ? "1" : "0");
                }
            }
            Names.push_back(B.Name);
            Out.push_back(std::move(B));
        }
        Gdt::Block Table{ Name, "weaponcamotable.gdf", "", {} };
        Table.Keys.emplace_back("configstringFileType", "WEAPONCAMOTABLE");
        Table.Keys.emplace_back("numCamoTables", std::to_string(Names.size()));
        for (int I = 0; I < Tables; I++)
            Table.Keys.emplace_back(std::format("table_{:02}_name", I + 1), I < (int)Names.size() ? Names[(size_t)I] : "");
        Out.push_back(std::move(Table));
        S.Counters["weaponcamo_camos"] += Count;
        return true;
    }
}

GDT_CONVERTER(t7_weaponcamo, Convert);
