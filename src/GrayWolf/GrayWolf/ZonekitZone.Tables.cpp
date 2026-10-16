#include "stdafx.h"

// ZonekitZone's string table kind (ZonekitZone.h): what every game whose zones hold string tables
// writes them with (ExportWriters::WriteStringTable), each game saying where its cells are.

#include "ZonekitZone.h"

#include "CoDAssets.h"
#include "WraithNameIndex.h"
#include "ExportNaming.h"
#include "ExportWriters.h"

#include <format>
#include <set>

void ZonekitZone::ExportStringTables(const DiskGame::Context& C, DiskGame::Totals& T, const char* Type, const StringTableReader& Read)
{
    using DiskGame::Kind;
    auto& Kt = T.Kinds[Kind::Data];
    std::vector<int64_t> Found;
    for (const auto& I : Walk().Instances)
        if (I.Parent < 0 && OwnerIs(I, Type)) Found.push_back(I.Pos);
    const std::string Dir = C.KindDir(Kind::Data);
    std::set<std::string> Written;
    for (size_t N = 0; N < Found.size(); N++)
    {
        const int64_t Pos = Found[N];
        const std::string Name = AssetName(Kind::Data, Pos);
        if (!C.Advance(N, Found.size(), Name)) break;
        if (IsReference(Kind::Data, Name, Pos))
        {
            Kt.References++;
            continue;
        }
        if (!C.Wants(Kind::Data, Name) || !Written.insert(ExportNaming::Lower(Name)).second) continue;
        Kt.Seen++;
        uint32_t Columns = 0, Rows = 0;
        std::vector<std::string> Cells;
        if (!Read(Pos, Columns, Rows, Cells))
        {
            T.Fail(ZoneName, Kind::Data, Name, "cells not read");
            continue;
        }
        Counter("stringtable_cells") += Cells.size();
        if (ExportWriters::WriteStringTable(Dir, Name, Columns, Rows, Cells)) Kt.Written++;
        else T.Fail(ZoneName, Kind::Data, Name, "write failed");
    }
    C.Advance(Found.size(), Found.size());
}

bool ZonekitZone::StringTableCells(int64_t Pos, const StringTableLayout& L, uint32_t& Columns, uint32_t& Rows,
                                   std::vector<std::string>& Cells) const
{
    const int32_t Cols = At<int32_t>(Image(), Pos + L.ColumnsAt), Count = At<int32_t>(Image(), Pos + L.RowsAt);
    Columns = Rows = 0;
    Cells.clear();
    if (Cols <= 0 || Count <= 0) return Cols >= 0 && Count >= 0;   // an empty table
    const uint64_t All = (uint64_t)Cols * (uint64_t)Count;
    const int64_t At = Off(PointerAt(Pos + L.CellsAt));
    if (At < 0 || All > (1u << 24) || (uint64_t)At + All * (uint64_t)L.Cell > Image().size()) return false;
    Columns = (uint32_t)Cols, Rows = (uint32_t)Count;
    Cells.reserve((size_t)All);
    for (uint64_t K = 0; K < All; K++) Cells.push_back(TextAt(At + (int64_t)(K * (uint64_t)L.Cell)));
    return true;
}

std::string ZonekitZone::KnownHash(uint64_t Hash) const
{
    if (Hash == 0) return std::string();
    std::string N = HashName(Hash);
    if (!N.empty()) return N;
    for (const auto* Db : { &CoDAssets::AssetNameCache.NameDatabase, &CoDAssets::StringCache.NameDatabase })
        for (const uint64_t Key : { Hash, Hash & 0x7FFFFFFFFFFFFFFFull, Hash & 0xFFFFFFFFFFFFFFFull })
            if (const auto It = Db->find(Key); It != Db->end()) return It->second;
    return std::string();
}

bool ZonekitZone::ColumnTableCells(int64_t Pos, ColumnTypes Types, const std::function<std::string(const std::string&)>& Text,
                                   uint32_t& Columns, uint32_t& Rows, std::vector<std::string>& Cells) const
{
    const int32_t Cols = At<int32_t>(Image(), Pos + 8), Count = At<int32_t>(Image(), Pos + 0xc);
    Columns = Rows = 0;
    Cells.clear();
    if (Cols <= 0 || Count <= 0) return Cols >= 0 && Count >= 0;
    const int64_t List = Off(PointerAt(Pos + 0x18));
    if (List < 0 || (uint64_t)Cols * (uint64_t)Count > (1u << 24) || (uint64_t)List + (uint64_t)Cols * 0x28 > Image().size()) return false;
    Columns = (uint32_t)Cols, Rows = (uint32_t)Count;
    Cells.assign((size_t)Columns * Rows, std::string());
    const bool Bo6 = Types == ColumnTypes::Bo6;
    const auto Hash = [this](const char* Prefix, uint64_t H) {
        const std::string N = KnownHash(H);
        return std::string(Prefix) + (N.empty() ? std::format("{:x}", (unsigned long long)H) : N);
    };
    for (uint32_t Col = 0; Col < Columns; Col++)
    {
        const int64_t Column = List + (int64_t)Col * 0x28;
        const uint8_t Type = At<uint8_t>(Image(), Column);
        const uint16_t Values = At<uint16_t>(Image(), Column + 2), Indexed = At<uint16_t>(Image(), Column + 4);
        const int64_t Index = Off(PointerAt(Column + 0x10)), Data = Off(PointerAt(Column + 0x20));
        // the element size by type: 4 for a float and a 32-bit hash, 1 for a bool, else 8
        const bool Hash32 = Type == (Bo6 ? 9 : 8);
        const int64_t Size = (Type == 3 || Hash32) ? 4 : Type == 4 ? 1 : 8;
        if (Index < 0 || Data < 0) continue;
        for (uint32_t Row = 0; Row < Rows && Row < Indexed; Row++)
        {
            const uint16_t V = At<uint16_t>(Image(), Index + (int64_t)Row * 2);
            if (V >= Values) continue;
            const int64_t At0 = Data + (int64_t)V * Size;
            if ((uint64_t)At0 + (uint64_t)Size > Image().size()) continue;
            const uint64_t U = At<uint64_t>(Image(), At0);
            std::string& Out = Cells[(size_t)Row * Columns + Col];
            switch (Type)
            {
            case 1: Out = Text(TextAt(At0)); break;
            case 2: Out = std::to_string(At<int64_t>(Image(), At0)); break;
            case 3: Out = std::format("{}", At<float>(Image(), At0)); break;
            case 4: Out = At<uint8_t>(Image(), At0) ? "TRUE" : "FALSE"; break;
            case 5: Out = Hash("#", U); break;
            case 6: Out = Hash("%#", U); break;
            case 7: Out = Hash("@#", U); break;
            default:
                if (Hash32) Out = Hash("x32#", At<uint32_t>(Image(), At0));
                else if (!Bo6 && Type == 9) Out = "&" + Text(TextAt(At0));
                else if (!Bo6 && Type == 10) Out = "a&" + Text(TextAt(At0));
                else if (Bo6 && Type == 8) Out = Hash("o#", U);
                else if (Bo6 && Type == 10) Out = Hash("s#", U);
                else if (Bo6 && Type == 11) Out = Hash("&#", U);
                else Out = std::format("unk{}:{:x}", Type, (unsigned long long)U);
            }
        }
    }
    return true;
}

void ZonekitZone::ExportsColumnTables(ColumnTypes Types, std::function<std::string(const std::string&)> Text)
{
    using DiskGame::Kind;
    Exports(Kind::Data, [this, Types, Text = std::move(Text)](const DiskGame::Context& C, DiskGame::Totals& T) {
        ExportStringTables(C, T, "stringtable", [&](int64_t Pos, uint32_t& Columns, uint32_t& Rows, std::vector<std::string>& Cells) {
            return ColumnTableCells(Pos, Types, Text, Columns, Rows, Cells);
        });
    });
}
