#include "stdafx.h"

// BO3 animmappingtable (76) -> its .ai_am file, as HydraX's AnimationMappingTable.cs
// (github.com/Scobalula/HydraX, GPL-3.0): header {name 0, maps 8, count 0x10}; a map (0x18) {alias
// script string 0, animations 8 (script strings), count 0x10}. Written in type.jsonc's "files"
// folder: a "#" line, then each alias and its animations, each followed by ",". No GDT entry: the
// Mod Tools' share/raw/animtables holds these files.

#include "Gdt.h"

namespace
{
    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>&, Gdt::Stats& S)
    {
        std::string Out = "#\r\n";
        const int64_t Maps = M.Follow(Pos + 8);
        const int32_t Count = M.At<int32_t>(Pos + 0x10);
        for (int32_t I = 0; Maps >= 0 && I < Count && I < 1 << 20; I++)
        {
            const int64_t Map = Maps + 0x18 * (int64_t)I;
            Out += M.ScriptString(M.At<uint32_t>(Map)) + ",";
            const int64_t Anims = M.Follow(Map + 8);
            const int32_t AnimCount = M.At<int32_t>(Map + 0x10);
            for (int32_t A = 0; Anims >= 0 && A < AnimCount && A < 65536; A++) Out += M.ScriptString(M.At<uint32_t>(Anims + 4 * (int64_t)A)) + ",";
            Out += "\r\n";
        }
        S.Files.emplace_back(T.Settings().value("files", std::string()) + Name, std::move(Out));
        return true;
    }
}

GDT_CONVERTER(t7_animmappingtable, Convert);
