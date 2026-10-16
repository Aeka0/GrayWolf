#include "stdafx.h"

// BO3 attachmentcosmeticvariant (89) -> its GDT entry, as HydraX's AttachmentCosmeticVariant.cs
// (github.com/Scobalula/HydraX, GPL-3.0): header {name 0, variants 8, count 0x10}; a variant (0xe0)
// is 4 models (0x30: model 0, ADS model 8, tag string 0x10, offset 0x18, angles 0x24; the first two
// view, the last two world), short name 0xc0, long name 0xc8, description 0xd0, icon image 0xd8.
// Every variant the GDF has (type.jsonc "variants") is written, the missing ones empty.

#include "Gdt.h"

#include <format>

namespace
{
    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out,
                 Gdt::Stats& S)
    {
        Gdt::Block B{ Name, "attachmentcosmeticvariant.gdf", "", {} };
        const int64_t List = M.Follow(Pos + 8);
        const int Count = List < 0 ? 0 : (int)M.At<uint32_t>(Pos + 0x10);
        const int Written = std::max(Count, T.Settings().value("variants", 0));
        const char* Kinds[] = { "viewModel", "worldModel" };
        for (int I = 0; I < Written && I < 64; I++)
        {
            const int64_t V = I < Count ? List + 0xe0 * (int64_t)I : -1;
            const auto Key = [&](const std::string& K) { return std::format("acv{}_{}", I, K); };
            for (int J = 0; J < 4; J++)
            {
                const int64_t Md = V < 0 ? -1 : V + 0x30 * J;
                const std::string Kind = Kinds[J < 2 ? 0 : 1], Model = std::format("_model{}", J % 2);
                const auto F = [&](int64_t At) { return Gdt::Number(Md < 0 ? 0.0 : M.At<float>(Md + At), false); };
                B.Keys.emplace_back(Key(Kind + Model), Md < 0 ? "" : M.AssetField(Md));
                B.Keys.emplace_back(Key(Kind + "ADS" + Model), Md < 0 ? "" : M.AssetField(Md + 8));
                B.Keys.emplace_back(Key(Kind + "Tag" + Model), Md < 0 ? "" : M.StringField(Md + 0x10));
                B.Keys.emplace_back(Key(Kind + "OffsetX" + Model), F(0x18));
                B.Keys.emplace_back(Key(Kind + "OffsetY" + Model), F(0x1c));
                B.Keys.emplace_back(Key(Kind + "OffsetZ" + Model), F(0x20));
                B.Keys.emplace_back(Key(Kind + "OffsetPitch" + Model), F(0x24));
                B.Keys.emplace_back(Key(Kind + "OffsetYaw" + Model), F(0x28));
                B.Keys.emplace_back(Key(Kind + "OffsetRoll" + Model), F(0x2c));
            }
            B.Keys.emplace_back(Key("description"), V < 0 ? "" : M.StringField(V + 0xd0));
            B.Keys.emplace_back(Key("displayNameShort"), V < 0 ? "" : M.StringField(V + 0xc0));
            B.Keys.emplace_back(Key("displayNameLong"), V < 0 ? "" : M.StringField(V + 0xc8));
            B.Keys.emplace_back(Key("uiMaterial"), V < 0 ? "" : M.AssetField(V + 0xd8, 0xf8));
        }
        S.Counters["acv_variants"] += Count;
        Out.push_back(std::move(B));
        return true;
    }
}

GDT_CONVERTER(t7_attachmentcosmeticvariant, Convert);
