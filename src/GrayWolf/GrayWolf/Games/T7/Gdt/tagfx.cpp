#include "stdafx.h"

// BO3 tagfx (39) -> its GDT entry, as HydraX's TagFX.cs (github.com/Scobalula/HydraX, GPL-3.0):
// header {name 0, count 8, items 0x10}; an item (0x70) {fx 0, tag script string 0x60, delay 0x64
// (ms), bolted 0x68}, written fx<n>, tag<n>, timeDelay<n>, bolted<n> for the count the entry holds.

#include "Gdt.h"

#include <filesystem>

namespace
{
    // <path>.efx, its folders' separators as \\: tagfx.gdf keeps it under share/raw/fx/ (HydraX's
    // CleanAssetName(FX) adds an fx\\ folder the Mod Tools do not write there)
    std::string FxPath(const std::string& N)
    {
        if (N.empty()) return N;
        std::string P = std::filesystem::path(N).replace_extension(".efx").string(), Out;
        for (char C : P) Out += (C == '/' || C == '\\') ? std::string("\\\\") : std::string(1, C);
        return Out;
    }

    bool Convert(const Gdt::Type&, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out,
                 Gdt::Stats& S)
    {
        Gdt::Block B{ Name, "tagfx.gdf", "", {} };
        const int64_t Items = M.Follow(Pos + 0x10);
        const int Count = Items < 0 ? 0 : (int)std::min<uint32_t>(M.At<uint32_t>(Pos + 8), 256);
        B.Keys.emplace_back("tagFXItemCount", std::to_string(Count));
        for (int I = 0; I < Count; I++)
        {
            const int64_t It = Items + 0x70 * (int64_t)I;
            const std::string N = std::to_string(I + 1);
            B.Keys.emplace_back("bolted" + N, std::to_string(M.At<int32_t>(It + 0x68)));
            B.Keys.emplace_back("fx" + N, FxPath(M.AssetField(It)));
            B.Keys.emplace_back("timeDelay" + N, std::to_string(M.At<int32_t>(It + 0x64)));
            B.Keys.emplace_back("tag" + N, M.ScriptString(M.At<uint32_t>(It + 0x60)));        }
        S.Counters["tagfx_items"] += Count;
        Out.push_back(std::move(B));
        return true;
    }
}

GDT_CONVERTER(t7_tagfx, Convert);
