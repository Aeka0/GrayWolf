#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct SeAnimClip;

// Reads the BO3 Mod Tools' `.xanim_bin` format - the compiled form of a `.XANIM_EXPORT` text file,
// produced by `export2bin.exe`, reverse-engineered from that tool.
//
// NOT the same format as the retail game's own compiled `XAnimParts` (the struct actually stored
// in retail `.ff` fast files). This reader targets the ~980 animations the Mod Tools ship on disk
// under `xanim_export/`.

// `valueType` from the doc's keyword table - selects the binary shape below. Numeric values match
// the real writer's switch cases exactly (see the doc's "valueType -> binary encoding" table).
// Most cases write 2 literal zero bytes right after the tag, before the payload described below:
// String/Marker/Raw32/Raw32_2/UInt8Nx4/Raw32_3/Raw32x2/Raw32x3/Raw32x4 do;
// Int16/UInt16/Int16Nx3/UVArray/UInt16String do not. XAnimBinReader.cpp's ReadRecord is
// the source of truth for which.
enum class XAnimBinValueType : uint32_t
{
    String = 0,          // Raw bytes, NUL-terminated then padded to 4-byte alignment.
    Marker = 1,           // No payload at all - the tag alone is the record (ANIMATION, MODEL, X, Y, Z).
    Int16 = 2,
    UInt16 = 3,
    Raw32 = 4,
    Raw32_2 = 5,          // Same wire shape as Raw32 (1 raw uint32/float), distinct keyword set.
    UInt8Nx4 = 6,          // Quantized 4x uint8: byte = uint8(value*255+0.5).
    Int16Nx3 = 7,          // Quantized 3x int16: value*32767+0.5, sign-flipped via XOR if negative.
    Raw32_3 = 8,
    Raw32x2 = 9,
    Raw32x3 = 10,
    Raw32x4 = 0xb,
    UInt16Float = 0xc,     // uint16 (bone index) + raw float (weight) - BONE's 2nd overload.
    UVArray = 0xd,         // uint16 count, then count x (float,float) pairs.
    UInt16String = 0xe,    // uint16 + string.
    UInt16String3 = 0xf,   // uint16 + 3 strings.
    UInt32String = 0x10,
    UInt32x2String = 0x11,
    UInt8x2 = 0x12,
    UInt16x2 = 0x13,
};

// One resolved field record from the tagged-record stream. Numeric sub-fields are kept as raw
// wire bytes in `Raw` (in field order, no re-interpretation - e.g. Raw32x3's 12 bytes may be
// uint32 or float depending on the keyword, the writer never distinguishes at this layer) so
// Layer 1 never has to guess int-vs-float; a consumer memcpy's out what each `Keyword` implies.
// String-bearing types (0/0xe/0xf/0x10/0x11) get their decoded strings in `Strings` instead, in
// field order, with `Raw` holding just that type's leading non-string sub-field(s) if any.
struct XAnimBinRecord
{
    std::string Keyword;              // Resolved keyword text (e.g. "BONE", "FRAME", "NUMFRAMES").
    XAnimBinValueType Type = XAnimBinValueType::Marker;
    uint32_t TableIndex = 0;          // Position in the 60-entry keyword table - lets a consumer
                                       // tell apart the 3 BONE overloads etc. (same keyword text,
                                       // different `Type` - see the doc's own note on this).
    std::vector<uint8_t> Raw;         // Fixed-shape numeric payload, wire order, wire bytes.
    std::vector<std::string> Strings; // String sub-fields, in field order (String/UInt16String/
                                       // UInt16String3/UInt32String/UInt32x2String only).
    std::vector<std::pair<float, float>> UVs; // UVArray only.
};

// Layer 1 (mechanical, from the decompiled writer): decompress
// the LZ4 container and walk the payload as a sequence of [2-byte CRC tag][type-coded payload]
// records, resolving every tag against the precomputed 60-keyword table. Returns false on a bad
// magic, a failed LZ4 decompress, or any record whose tag doesn't match a known keyword (a real
// desync, not a soft stop - this format has no resync point once a record's length is misjudged).
bool ReadXAnimBinRecords(const std::string& Path, std::vector<XAnimBinRecord>& Out);

// Layer 2: assembles the tagged-record stream into a SeAnimClip - PART declarations become the
// bone list, FRAME/PART(select)/OFFSET/X/Y/Z become per-frame per-bone pos+rot keys (X/Y/Z are 3
// rows of a rotation matrix, converted to a quaternion here - see the .cpp for the exact
// convention and its caveats), NUMKEYS + FRAME(0x10)'s uint32-frame+string pairs become
// `Out.Notifications`. Not verified against a decompiled parser or a text source: the grammar is
// inferred from XANIM_EXPORT convention and checked only via structural invariants (declared
// NUMPARTS/NUMFRAMES/NUMKEYS must match what is assembled, else the read fails).
bool ReadXAnimBinClip(const std::string& Path, SeAnimClip& Out);
