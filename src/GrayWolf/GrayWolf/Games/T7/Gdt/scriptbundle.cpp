#include "stdafx.h"

// BO3 scriptbundle (69) -> its GDT entry, as HydraX's ScriptBundle.cs (github.com/Scobalula/HydraX,
// GPL-3.0): header {name 0, type script string 8, key/values count 0x10 and list 0x18, objects
// count 0x20 and list 0x28}; a key/value (24 bytes) {key script string 0, value script string 4};
// an object (16 bytes) {count 0 (u64), key/values 8}, written object<n>_<key>. The GDF is the
// bundle's type (none: accolade; scene: scriptbundle). The game keeps keys lowercase: the Mod
// Tools' case is restored from properties.json (HydraX's table, beside type.jsonc), an xstring_
// key from its name without the prefix.

#include "Gdt.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>

namespace
{
    using Keys = std::map<std::string, std::map<std::string, std::string>>;   // type -> lower key -> key

    const Keys& KeysOf(const Gdt::Type& T)
    {
        static std::mutex Lock;
        static std::map<std::string, Keys> Read;
        std::lock_guard<std::mutex> Guard(Lock);
        auto It = Read.find(T.Folder());
        if (It != Read.end()) return It->second;
        Keys K;
        std::ifstream In(std::filesystem::path(T.Folder()) / "properties.json", std::ios::binary);
        if (In)
            try
            {
                const auto J = nlohmann::json::parse(In);
                for (const auto& [Type, Names] : J.items())
                    for (const auto& N : Names)
                    {
                        const std::string Key = N.get<std::string>();
                        std::string L;
                        for (char C : Key) L.push_back((char)std::tolower((unsigned char)C));
                        K[Type].emplace(L, Key);
                        if (L.rfind("xstring_", 0) == 0) K[Type].emplace(L.substr(8), Key);
                    }
            }
            catch (const std::exception&)
            {
            }
        return Read.emplace(T.Folder(), std::move(K)).first->second;
    }

    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out,
                 Gdt::Stats& S)
    {
        std::string Type = M.ScriptString(M.At<uint32_t>(Pos + 8));
        if (Type.empty()) Type = "accolade";
        else if (Type == "scene") Type = "scriptbundle";
        Gdt::Block B{ Name, Type + ".gdf", "", {} };
        const Keys& All = KeysOf(T);
        const auto Known = All.find(Type);
        const auto Pairs = [&](int64_t List, uint64_t Count, const std::string& Prefix) {
            for (uint64_t I = 0; List >= 0 && I < Count && I < 65536; I++)
            {
                const int64_t P = List + 24 * (int64_t)I;
                std::string Key = M.ScriptString(M.At<uint32_t>(P)), Value = M.ScriptString(M.At<uint32_t>(P + 4));
                std::string L;
                for (char C : Key) L.push_back((char)std::tolower((unsigned char)C));
                if (Known != All.end())
                    if (const auto K = Known->second.find(L); K != Known->second.end()) Key = K->second;
                for (size_t At = Value.find("scriptvector"); At != std::string::npos; At = Value.find("scriptvector", At))
                    Value.replace(At, 12, "scriptVector");
                if (Key == "configstringFileType")   // the game keeps it lowercase, the Mod Tools write it upper
                    for (char& C : Value) C = (char)std::toupper((unsigned char)C);
                B.Keys.emplace_back(Prefix + Key, Value);
            }
        };
        Pairs(M.Follow(Pos + 0x18), M.At<uint32_t>(Pos + 0x10), "");
        const int64_t Objects = M.Follow(Pos + 0x28);
        const uint32_t ObjectCount = M.At<uint32_t>(Pos + 0x20);
        for (uint32_t I = 0; Objects >= 0 && I < ObjectCount && I < 4096; I++)
        {
            const int64_t O = Objects + 16 * (int64_t)I;
            Pairs(M.Follow(O + 8), M.At<uint64_t>(O), "object" + std::to_string(I + 1) + "_");
        }
        // a key twice (the game's lowercase keys meeting HydraX's case): the last, as HydraX's dictionary
        std::map<std::string, size_t> Last;
        for (size_t I = 0; I < B.Keys.size(); I++) Last[B.Keys[I].first] = I;
        std::vector<std::pair<std::string, std::string>> Once;
        for (size_t I = 0; I < B.Keys.size(); I++)
            if (Last[B.Keys[I].first] == I) Once.push_back(std::move(B.Keys[I]));
        B.Keys = std::move(Once);
        S.Counters["scriptbundle_" + Type]++;
        Out.push_back(std::move(B));
        return true;
    }
}

GDT_CONVERTER(t7_scriptbundle, Convert);
