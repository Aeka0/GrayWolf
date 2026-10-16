#pragma once

#include <cstdint>
#include <string>
#include <vector>

// StringTable (type 48) cell capture. See Bo3ZoneDataTables.cpp.
struct Bo3StringTableCapture
{
    bool Valid = false;
    uint32_t Columns = 0;
    uint32_t Rows = 0;
    // Columns*Rows entries, in the same order the on-disk Values array is walked in (index i).
    // Empty string for a cell whose Text pointer was NOT FollowsInline - an already-resolved
    // reference this reader never follows.
    std::vector<std::string> Cells;
};

// One StructuredTable (type 49) cell, see Bo3StructuredTableCapture below.
struct Bo3StructuredTableCell
{
    // The cell type field (+0x00) - 1 means a STRING cell; carried raw rather than assumed.
    int32_t Type = 0;
    // Populated only for a real inline string cell (Text pointer read FollowsInline) - empty for
    // a non-string cell, or a string cell holding an already-resolved reference instead (see
    // misc-data-and-files.md's own note: roughly half of real STRING cells are resolved refs).
    std::string Text;
    // The raw number/hash field (+0x10) - meaningful for a non-string cell, ignored for a real
    // inline string cell.
    int32_t Number = 0;
};

// StructuredTable (type 49) capture. See Bo3ZoneDataTables.cpp.
struct Bo3StructuredTableCapture
{
    bool Valid = false;
    int32_t CellCount = 0;
    int32_t ColumnCount = 0;
    int32_t RowCount = 0;
    // ColumnCount entries, the real column names (e.g. "id", "objective_desc") - empty string for
    // a header whose string pointer was not inline.
    std::vector<std::string> Headers;
    // CellCount entries, in on-disk order.
    std::vector<Bo3StructuredTableCell> Cells;
};

// Ddl (type 51) schema capture, mirroring the game's byte layout one-for-one. See
// Bo3ZoneDataTables.cpp.
struct Bo3DdlMemberCapture
{
    std::string Name;
};

struct Bo3DdlStructDefCapture
{
    std::string Name;
    std::vector<Bo3DdlMemberCapture> Members;
};

struct Bo3DdlEnumDefCapture
{
    std::string Name;
    // The enum's own member names (deferred, one FollowsInline gate per entry).
    std::vector<std::string> Members;
};

// One `ddlDef` chain node (the real reader can chain several near-identical schema revisions
// together via NextPtr, see the doc section above) - one struct/enum def list each.
struct Bo3DdlDefNodeCapture
{
    std::vector<Bo3DdlStructDefCapture> Structs;
    std::vector<Bo3DdlEnumDefCapture> Enums;
};

struct Bo3DdlCapture
{
    bool Valid = false;
    std::vector<Bo3DdlDefNodeCapture> Chain;
};

// One table/schema captured from the zonekit-walked positions (Games/T7/DiskGame.cpp's
// ExportData), written as CSV/JSON. True when written; Ddl also adds its own struct/enum totals.
bool Bo3WriteStringTableCsv(const std::string& Name, const Bo3StringTableCapture& T, const std::string& OutDir);
bool Bo3WriteStructuredTableCsv(const std::string& Name, const Bo3StructuredTableCapture& T, const std::string& OutDir);
bool Bo3WriteDdlJson(const std::string& Name, const Bo3DdlCapture& Ddl, const std::string& OutDir,
                     size_t& StructsTotal, size_t& EnumsTotal);
