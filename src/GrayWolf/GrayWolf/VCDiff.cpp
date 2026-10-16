#include "stdafx.h"
#include "VCDiff.h"

#include <array>
#include <cstring>

namespace VCDiff
{
    namespace
    {
        enum class Inst : uint8_t { Noop = 0, Add = 1, Run = 2, Copy = 3 };

        struct OpEntry
        {
            Inst Inst1 = Inst::Noop, Inst2 = Inst::Noop;
            uint8_t Size1 = 0, Size2 = 0;
            uint8_t Mode1 = 0, Mode2 = 0;
        };

        // The RFC 3284 default code table (section 5.6), generated the same way the RFC itself
        // derives it, not hand-transcribed; matches open-vcdiff's kDefaultCodeTableData (src/codetable.cc).
        std::array<OpEntry, 256> BuildDefaultCodeTable()
        {
            std::array<OpEntry, 256> T{};

            T[0] = { Inst::Run, Inst::Noop, 0, 0, 0, 0 };

            for (int i = 0; i < 18; i++)
            {
                T[1 + i] = { Inst::Add, Inst::Noop, (uint8_t)i, 0, 0, 0 };
            }

            constexpr uint8_t CopySizes[16] = { 0, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };
            for (int mode = 0; mode < 9; mode++)
            {
                for (int j = 0; j < 16; j++)
                {
                    T[19 + mode * 16 + j] = { Inst::Copy, Inst::Noop, CopySizes[j], 0, (uint8_t)mode, 0 };
                }
            }

            // ADD (size 1..4) + COPY (mode 0..5, size 4..6)
            for (int mode = 0; mode < 6; mode++)
            {
                for (int sub = 0; sub < 12; sub++)
                {
                    const uint8_t AddSize = (uint8_t)(sub / 3 + 1);
                    const uint8_t CopySize = (uint8_t)(4 + sub % 3);
                    T[163 + mode * 12 + sub] = { Inst::Add, Inst::Copy, AddSize, CopySize, 0, (uint8_t)mode };
                }
            }

            // ADD (size 1..4) + COPY (mode 6..8, size 4)
            for (int mode = 0; mode < 3; mode++)
            {
                for (int sub = 0; sub < 4; sub++)
                {
                    T[235 + mode * 4 + sub] = { Inst::Add, Inst::Copy, (uint8_t)(sub + 1), 4, 0, (uint8_t)(6 + mode) };
                }
            }

            // COPY (mode 0..8, size 4) + ADD (size 1)
            for (int mode = 0; mode < 9; mode++)
            {
                T[247 + mode] = { Inst::Copy, Inst::Add, 4, 1, (uint8_t)mode, 0 };
            }

            return T;
        }

        const std::array<OpEntry, 256> DefaultCodeTable = BuildDefaultCodeTable();

        // Reads a base-128 big-endian varint (RFC 3284 section 3.1): 7 payload bits per byte,
        // MSB set means "more bytes follow" (open-vcdiff's varint_bigendian.cc).
        bool ReadVarint(const uint8_t*& Pos, const uint8_t* End, uint64_t& Out)
        {
            uint64_t Result = 0;
            for (; Pos < End; Pos++)
            {
                Result = (Result << 7) | (*Pos & 0x7F);
                if (!(*Pos & 0x80))
                {
                    Pos++;
                    Out = Result;
                    return true;
                }
            }
            return false;
        }

        bool ReadByte(const uint8_t*& Pos, const uint8_t* End, uint8_t& Out)
        {
            if (Pos >= End) return false;
            Out = *Pos++;
            return true;
        }

        // The NEAR and SAME address caches (RFC 3284 section 5.1), default sizes 4 and 3.
        struct AddressCache
        {
            static constexpr uint8_t NearSize = 4;
            static constexpr uint8_t SameSize = 3;
            static constexpr uint8_t FirstNearMode = 2;
            static constexpr uint8_t FirstSameMode = FirstNearMode + NearSize; // 6

            int32_t Near[NearSize] = {};
            int32_t Same[SameSize * 256] = {};
            int NextSlot = 0;

            void Reset()
            {
                std::memset(Near, 0, sizeof(Near));
                std::memset(Same, 0, sizeof(Same));
                NextSlot = 0;
            }

            void Update(int32_t Address)
            {
                Near[NextSlot] = Address;
                NextSlot = (NextSlot + 1) % NearSize;
                Same[(uint32_t)Address % (SameSize * 256)] = Address;
            }

            // Returns false only on a malformed address stream (not enough bytes).
            bool Decode(int32_t HereAddress, uint8_t Mode, const uint8_t*& AddrPos, const uint8_t* AddrEnd, int32_t& Out)
            {
                if (Mode >= FirstSameMode && Mode < FirstSameMode + SameSize)
                {
                    uint8_t B = 0;
                    if (!ReadByte(AddrPos, AddrEnd, B)) return false;
                    Out = Same[(uint32_t)((Mode - FirstSameMode) * 256 + B)];
                }
                else
                {
                    uint64_t V = 0;
                    if (!ReadVarint(AddrPos, AddrEnd, V)) return false;
                    if (Mode == 0) // VCD_SELF
                    {
                        Out = (int32_t)V;
                    }
                    else if (Mode == 1) // VCD_HERE
                    {
                        Out = HereAddress - (int32_t)V;
                    }
                    else if (Mode >= FirstNearMode && Mode < FirstSameMode)
                    {
                        Out = Near[Mode - FirstNearMode] + (int32_t)V;
                    }
                    else
                    {
                        return false;
                    }
                }
                Update(Out);
                return true;
            }
        };
    }

    bool Decode(const std::vector<uint8_t>& Source, const uint8_t* Delta, size_t DeltaSize,
        std::vector<uint8_t>& Target, std::string& Error)
    {
        Target.clear();

        const uint8_t* Pos = Delta;
        const uint8_t* End = Delta + DeltaSize;

        if (DeltaSize < 5 || Pos[0] != 0xD6 || Pos[1] != 0xC3 || Pos[2] != 0xC4)
        {
            Error = "not a VCDIFF stream (bad magic)";
            return false;
        }
        const uint8_t HdrIndicator = Pos[4];
        Pos += 5;

        if (HdrIndicator & 0x01) // VCD_DECOMPRESS - secondary compressor
        {
            Error = "VCDIFF secondary compression is not supported";
            return false;
        }
        if (HdrIndicator & 0x02) // VCD_CODETABLE - custom code table
        {
            Error = "VCDIFF custom code tables are not supported";
            return false;
        }

        while (Pos < End)
        {
            uint8_t WinIndicator = 0;
            if (!ReadByte(Pos, End, WinIndicator)) { Error = "truncated window header"; return false; }

            size_t SourceSegLength = 0, SourceSegPosition = 0;
            const uint8_t* SourceSegPtr = nullptr;
            bool SegInTarget = false;   // VCD_TARGET: re-pointed after Target grows

            if (WinIndicator & 0x03) // VCD_SOURCE or VCD_TARGET
            {
                uint64_t Len = 0, PosVal = 0;
                if (!ReadVarint(Pos, End, Len) || !ReadVarint(Pos, End, PosVal))
                {
                    Error = "truncated source segment length/position";
                    return false;
                }
                SourceSegLength = (size_t)Len;
                SourceSegPosition = (size_t)PosVal;

                if (WinIndicator & 0x01) // VCD_SOURCE: from the external dictionary/source buffer
                {
                    if (SourceSegPosition + SourceSegLength > Source.size())
                    {
                        Error = "source segment runs past the end of the source buffer";
                        return false;
                    }
                    SourceSegPtr = Source.data() + SourceSegPosition;
                }
                else // VCD_TARGET: from the already-decoded output of this same stream
                {
                    if (SourceSegPosition + SourceSegLength > Target.size())
                    {
                        Error = "VCD_TARGET source segment runs past already-decoded output";
                        return false;
                    }
                    SegInTarget = true;
                }
            }

            // BO4 .fd: window bit 0x04 adds one varint here (not needed to decode), bit 0x08
            // puts 4 bytes after the window.
            if (WinIndicator & 0x04)
            {
                uint64_t Unused = 0;
                if (!ReadVarint(Pos, End, Unused)) { Error = "truncated window extra field"; return false; }
            }

            uint64_t DeltaEncodingLength = 0;
            if (!ReadVarint(Pos, End, DeltaEncodingLength)) { Error = "truncated delta encoding length"; return false; }
            const uint8_t* DeltaEncodingStart = Pos;

            uint64_t TargetWindowLength = 0;
            if (!ReadVarint(Pos, End, TargetWindowLength)) { Error = "truncated target window length"; return false; }

            uint8_t DeltaIndicator = 0;
            if (!ReadByte(Pos, End, DeltaIndicator)) { Error = "truncated delta indicator"; return false; }
            if (DeltaIndicator & 0x07)
            {
                Error = "VCDIFF secondary section compression is not supported";
                return false;
            }

            uint64_t DataLen = 0, InstLen = 0, AddrLen = 0;
            if (!ReadVarint(Pos, End, DataLen) || !ReadVarint(Pos, End, InstLen) || !ReadVarint(Pos, End, AddrLen))
            {
                Error = "truncated section lengths";
                return false;
            }

            const uint8_t* DataSec = Pos;
            const uint8_t* DataSecEnd = DataSec + DataLen;
            const uint8_t* InstSec = DataSecEnd;
            const uint8_t* InstSecEnd = InstSec + InstLen;
            const uint8_t* AddrSec = InstSecEnd;
            const uint8_t* AddrSecEnd = AddrSec + AddrLen;

            if (AddrSecEnd > End || AddrSecEnd != DeltaEncodingStart + DeltaEncodingLength)
            {
                Error = "window section lengths do not add up to the declared delta encoding length";
                return false;
            }

            AddressCache Cache;
            Cache.Reset();

            const size_t TargetWindowStart = Target.size();
            Target.resize(TargetWindowStart + (size_t)TargetWindowLength);
            if (SegInTarget) SourceSegPtr = Target.data() + SourceSegPosition;
            size_t Written = 0;

            const uint8_t* DataPos = DataSec;
            const uint8_t* InstPos = InstSec;
            const uint8_t* AddrPos = AddrSec;

            while (Written < (size_t)TargetWindowLength)
            {
                uint8_t Opcode = 0;
                if (!ReadByte(InstPos, InstSecEnd, Opcode)) { Error = "truncated instruction stream"; return false; }
                const OpEntry& Entry = DefaultCodeTable[Opcode];

                for (int Half = 0; Half < 2; Half++)
                {
                    const Inst I = Half == 0 ? Entry.Inst1 : Entry.Inst2;
                    if (I == Inst::Noop) continue;
                    uint8_t Mode = Half == 0 ? Entry.Mode1 : Entry.Mode2;
                    uint8_t FixedSize = Half == 0 ? Entry.Size1 : Entry.Size2;

                    size_t Size = FixedSize;
                    if (FixedSize == 0)
                    {
                        uint64_t V = 0;
                        if (!ReadVarint(InstPos, InstSecEnd, V)) { Error = "truncated instruction size"; return false; }
                        Size = (size_t)V;
                    }
                    if (Written + Size > (size_t)TargetWindowLength)
                    {
                        Error = "instruction size exceeds target window length";
                        return false;
                    }

                    uint8_t* Out = Target.data() + TargetWindowStart + Written;

                    if (I == Inst::Add)
                    {
                        if (DataPos + Size > DataSecEnd) { Error = "ADD runs past data section"; return false; }
                        std::memcpy(Out, DataPos, Size);
                        DataPos += Size;
                    }
                    else if (I == Inst::Run)
                    {
                        if (DataPos + 1 > DataSecEnd) { Error = "RUN runs past data section"; return false; }
                        std::memset(Out, *DataPos, Size);
                        DataPos += 1;
                    }
                    else // Copy
                    {
                        const int32_t HereAddress = (int32_t)(SourceSegLength + Written);
                        int32_t Address = 0;
                        if (!Cache.Decode(HereAddress, Mode, AddrPos, AddrSecEnd, Address))
                        {
                            Error = "truncated or invalid COPY address";
                            return false;
                        }
                        if (Address < 0 || Address >= HereAddress) { Error = "COPY address out of range"; return false; }

                        // The combined addressable range is [source segment][target bytes
                        // decoded so far in this window]; a copy can span across that boundary,
                        // and can even read bytes this same instruction is still writing (RFC
                        // 3284's self-referential run-length trick) - copy byte by byte instead
                        // of memcpy/memmove to get that overlap right.
                        if ((size_t)Address + Size <= SourceSegLength)
                        {
                            std::memcpy(Out, SourceSegPtr + Address, Size);
                            Written += Size;
                            continue;
                        }
                        for (size_t k = 0; k < Size; k++)
                        {
                            const size_t A = (size_t)Address + k;
                            uint8_t V;
                            if (A < SourceSegLength)
                            {
                                V = SourceSegPtr[A];
                            }
                            else
                            {
                                const size_t TargetRelative = A - SourceSegLength;
                                V = *(Target.data() + TargetWindowStart + TargetRelative);
                            }
                            Out[k] = V;
                        }
                    }

                    Written += Size;
                }
            }

            if (Written != (size_t)TargetWindowLength)
            {
                Error = "decoded window size does not match the declared target window length";
                return false;
            }

            Pos = AddrSecEnd;
            if (WinIndicator & 0x08)
            {
                if (End - Pos < 4) { Error = "truncated window trailer"; return false; }
                Pos += 4;
            }
        }

        return true;
    }
}
