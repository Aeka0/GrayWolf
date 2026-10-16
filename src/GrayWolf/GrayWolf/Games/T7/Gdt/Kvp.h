#pragma once

// What BO3's converters writing several entries share (customizationtable, maptable).
// Key/value property blocks (HydraX's KVPBlock, github.com/Scobalula/HydraX, GPL-3.0), which a
// customization table's body types and a map table's maps carry: 24 bytes {name script string 0,
// value script string 4, data type 8, int data 0xc, data pointer 0x10}. Written <type>_<name> as the
// Mod Tools' GDFs name them (xstring_, int_, image_...); an int whose pointer's low half is not its
// value is a bool (HydraX's reading); an int (or bool) writes its number, the rest the value string.

#include "Gdt.h"

#include <string>
#include <utility>
#include <vector>

namespace T7Gdt
{
    // Fx: an fx value written as the Mod Tools' fx path (fx\\<path>.efx), as HydraX's customization table
    inline void KeyValues(const Gdt::Memory& M, int64_t List, uint32_t Count, bool Fx,
                          std::vector<std::pair<std::string, std::string>>& Out)
    {
        enum : int32_t { XString = 0, Int = 1, Float = 2, Anim = 3, PlayerAnim = 4, XModel = 6, FxType = 9, XCam = 0xd, Image = 0xf, Bool = 0x10 };
        for (uint32_t I = 0; List >= 0 && I < Count && I < 4096; I++)
        {
            const int64_t P = List + 24 * (int64_t)I;
            const int32_t Type = M.At<int32_t>(P + 8), Data = M.At<int32_t>(P + 0xc);
            int32_t Shown = Type;
            if (Type == Int && (uint32_t)(M.At<uint64_t>(P + 0x10) & 0xffffffffull) != (uint32_t)Data) Shown = Bool;
            std::string Prefix;
            switch (Shown)
            {
            case XString: Prefix = "xstring"; break;
            case Int: Prefix = "int"; break;
            case Float: Prefix = "float"; break;
            case Anim: Prefix = "anim"; break;
            case PlayerAnim: Prefix = "playeranim"; break;
            case XModel: Prefix = "xmodel"; break;
            case FxType: Prefix = "fx"; break;
            case XCam: Prefix = "xcam"; break;
            case Image: Prefix = "image"; break;
            case Bool: Prefix = "bool"; break;
            default: Prefix = std::to_string(Shown); break;   // HydraX: an enum's number
            }
            const std::string Name = M.ScriptString(M.At<uint32_t>(P));
            std::string Value = M.ScriptString(M.At<uint32_t>(P + 4));
            if (Fx && Shown == FxType)
            {
                std::string Path = "fx\\\\";
                for (char C : Value) Path += C == '/' ? std::string("\\\\") : std::string(1, C);
                Value = Path + ".efx";
            }
            Out.emplace_back(Prefix + "_" + Name, Type == Int ? std::to_string(Data) : Value);
        }
    }

    // a name an entry can carry: printable, no quote (a misplaced string in a zone whose layout has
    // gaps reads bytes that would break the .gdt)
    inline bool EntryName(const std::string& N)
    {
        if (N.empty()) return false;
        for (unsigned char C : N)
            if (C < 0x20 || C >= 0x7f || C == '"') return false;
        return true;
    }

    // the keys once, the last value of a key written twice (HydraX's dictionary), in first-seen order
    inline void Once(std::vector<std::pair<std::string, std::string>>& Keys)
    {
        std::vector<std::pair<std::string, std::string>> Out;
        for (auto& [K, V] : Keys)
        {
            bool Seen = false;
            for (auto& P : Out)
                if (P.first == K)
                {
                    P.second = std::move(V);
                    Seen = true;
                    break;
                }
            if (!Seen) Out.emplace_back(std::move(K), std::move(V));
        }
        Keys = std::move(Out);
    }
}
