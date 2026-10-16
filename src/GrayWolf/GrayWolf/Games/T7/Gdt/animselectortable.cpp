#include "stdafx.h"

// BO3 animselectortable (75) -> its .ai_ast file, as HydraX's AnimationSelectorTable.cs
// (github.com/Scobalula/HydraX, GPL-3.0): header {name 0, selectors 8, count 0x10}; a selector
// (0x58) {name script string 0, columns 8, count 0x10, rows 0x18, count 0x20}; a column (0x18) {name
// script string 0, data type 0xc}; a row (0x10) {cells 0, count 8}; a cell (8) {script string 0,
// int or float 4}. Written in type.jsonc's "files" folder, each selector as its name, its columns'
// names, its rows and a "," line; a cell's string uppercased (or "*" for an empty string/enum one),
// else its number. No GDT entry: the Mod Tools' share/raw/animtables holds these files.

#include "Gdt.h"
#include "HydraText.h"

#include <cctype>

namespace
{
    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>&, Gdt::Stats& S)
    {
        const auto Str = [&](int64_t P) { return M.ScriptString(M.At<uint32_t>(P)); };
        std::string Out;
        const int64_t Selectors = M.Follow(Pos + 8);
        const int32_t Count = M.At<int32_t>(Pos + 0x10);
        for (int32_t I = 0; Selectors >= 0 && I < Count && I < 65536; I++)
        {
            const int64_t Sel = Selectors + 0x58 * (int64_t)I;
            Out += Str(Sel) + "\r\n";
            const int64_t Columns = M.Follow(Sel + 8);
            const int32_t ColumnCount = M.At<int32_t>(Sel + 0x10);
            std::vector<int32_t> Types;
            for (int32_t C = 0; Columns >= 0 && C < ColumnCount && C < 4096; C++)
            {
                Out += Str(Columns + 0x18 * (int64_t)C) + ",";
                Types.push_back(M.At<int32_t>(Columns + 0x18 * (int64_t)C + 0xc));
            }
            Out += "\r\n";
            const int64_t Rows = M.Follow(Sel + 0x18);
            const int32_t RowCount = M.At<int32_t>(Sel + 0x20);
            for (int32_t R = 0; Rows >= 0 && R < RowCount && R < 1 << 20; R++)
            {
                const int64_t Cells = M.Follow(Rows + 0x10 * (int64_t)R);
                for (size_t C = 0; C < Types.size(); C++)
                {
                    const int64_t Cell = Cells < 0 ? -1 : Cells + 8 * (int64_t)C;
                    std::string V = Cell < 0 ? std::string() : Str(Cell);
                    for (char& Ch : V) Ch = (char)std::toupper((unsigned char)Ch);
                    switch (Types[C])
                    {
                    case 0:   // enumerator
                    case 6:   // string
                        Out += (V.empty() ? std::string("*") : V) + ",";
                        break;
                    case 2:   // int
                        Out += (V.empty() ? std::to_string((Cell < 0 ? 0 : M.At<int32_t>(Cell + 4))) : V) + ",";
                        break;
                    case 1:   // float, its min and max
                    case 3:
                    case 4:
                        Out += (V.empty() ? HydraText::Float((Cell < 0 ? 0.0f : M.At<float>(Cell + 4))) : V) + ",";
                        break;
                    default:   // a type HydraX does not write
                        S.Counters["animselectortable_column_type_" + std::to_string(Types[C])]++;
                        break;
                    }
                }
                Out += "\r\n";
            }
            Out += ",\r\n";
        }
        S.Files.emplace_back(T.Settings().value("files", std::string()) + Name, std::move(Out));
        return true;
    }
}

GDT_CONVERTER(t7_animselectortable, Convert);
