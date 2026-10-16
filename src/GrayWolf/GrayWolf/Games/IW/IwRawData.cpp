#include "stdafx.h"

// The rawfiles, Lua files and string tables of the Infinity Ward zones from Ghosts to WWII
// (IWZone.h, ExportsIwRawfiles and ExportsIwStringTables), on ZonekitZone's rawfile and string table kinds. The
// shapes are the games' own readers' (their zonekit tables): rawfile 0x18 {name, u32 compressed
// size 8 (the bytes read), u32 size 0xc, buffer 0x10}; luafile 0x18 {name, i32 size 8, buffer
// 0x10}; stringtable 0x18 {name, i32 columns 8, rows 0xc, cells 0x10 of 0x10 {string, hash}}.

#include "Games/IW/IWZone.h"

void IWZone::ExportsIwRawfiles(const char* LuaType)
{
    using DiskGame::Kind;
    Exports(Kind::Rawfile, [this, LuaType](const DiskGame::Context& C, DiskGame::Totals& T) {
        ExportRawfiles(C, T,
                       { { "rawfile", [this](int64_t Pos, std::string&, std::vector<uint8_t>& Bytes) {
                              return RawBytes(Pos, { 0xc, 0x10, 8 }, Bytes);
                          } },
                         { LuaType, [this](int64_t Pos, std::string& Name, std::vector<uint8_t>& Bytes) {
                              if (!RawBytes(Pos, { 8, 0x10 }, Bytes)) return false;
                              NameLua(Name, Bytes);
                              return true;
                          } } });
    });
}

void IWZone::ExportsIwStringTables()
{
    using DiskGame::Kind;
    Exports(Kind::Data, [this](const DiskGame::Context& C, DiskGame::Totals& T) {
        ExportStringTables(C, T, "stringtable", [this](int64_t Pos, uint32_t& Columns, uint32_t& Rows, std::vector<std::string>& Cells) {
            return StringTableCells(Pos, {}, Columns, Rows, Cells);
        });
    });
}
