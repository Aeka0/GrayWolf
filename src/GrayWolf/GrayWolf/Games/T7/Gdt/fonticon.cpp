#include "stdafx.h"

// BO3 fonticon (21) -> its .csv, as HydraX's fontIcon.cs (github.com/Scobalula/HydraX, GPL-3.0):
// header {name 0, entries count 8, aliases count 0xc, entries 0x10, aliases 0x18}; an entry (0x28)
// {name string 0, its hash 8, material 0x10, size 0x18, x scale 0x1c, y scale 0x20}. A line per
// entry "name,size,material,xscale,yscale" (an unnamed one its hash, 0x%08X), then a line per alias
// "Alias,0x<alias hash>,0x<button hash>". The aliases are type.jsonc's "alias_size" bytes apart:
// the game loads 12 per alias (count * 3 u32s) where HydraX reads 8. Written as type.jsonc's "files"
// folder + the name, its extension made .csv. No GDT entry.

#include "Gdt.h"
#include "HydraText.h"

#include <algorithm>
#include <format>

namespace
{
    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>&, Gdt::Stats& S)
    {
        std::string Out;
        const int32_t Entries = std::clamp(M.At<int32_t>(Pos + 8), 0, 1 << 20), Aliases = std::clamp(M.At<int32_t>(Pos + 0xc), 0, 1 << 20);
        const int64_t EntryList = M.Follow(Pos + 0x10), AliasList = M.Follow(Pos + 0x18);
        for (int32_t I = 0; EntryList >= 0 && I < Entries; I++)
        {
            const int64_t E = EntryList + 0x28 * (int64_t)I;
            std::string Icon = M.At<uint64_t>(E) ? M.StringField(E) : std::format("0x{:08X}", M.At<uint32_t>(E + 8));
            Out += std::format("{},{},{},{},{}\r\n", Icon, M.At<int32_t>(E + 0x18), M.AssetField(E + 0x10),
                               HydraText::Float(M.At<float>(E + 0x1c)), HydraText::Float(M.At<float>(E + 0x20)));
        }
        const int64_t Stride = T.Settings().value("alias_size", 8);
        for (int32_t I = 0; AliasList >= 0 && I < Aliases; I++)
        {
            const int64_t A = AliasList + Stride * (int64_t)I;
            Out += std::format("Alias,0x{:08X},0x{:08X}\r\n", M.At<uint32_t>(A), M.At<uint32_t>(A + 4));
        }
        std::string File = T.Settings().value("files", std::string()) + Name;
        const size_t Slash = File.find_last_of("/\\"), Dot = File.find_last_of('.');
        if (Dot != std::string::npos && (Slash == std::string::npos || Dot > Slash)) File.erase(Dot);
        S.Files.emplace_back(File + ".csv", std::move(Out));
        return true;
    }
}

GDT_CONVERTER(t7_fonticon, Convert);
