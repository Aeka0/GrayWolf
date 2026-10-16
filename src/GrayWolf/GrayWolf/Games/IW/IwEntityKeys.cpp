#include "stdafx.h"

#include "IwEntityKeys.h"
#include "IwEntityKeyNames.h"

#include <format>
#include <mutex>
#include <unordered_map>

namespace IwEntityKeys
{
uint64_t Hash(std::string_view Name, Scheme S)
{
    constexpr uint64_t Prime = 0x10000000233ull;
    constexpr std::string_view Salt = "zt@f3yp(d[kkd=_@";
    uint64_t H = S == Scheme::Fields ? 0x79D6530B0BB9B5D1ull : 0x1C2F2E3C8A257D07ull;
    const auto Step = [&](char C) {
        const unsigned char U = (unsigned char)C;
        H = (H ^ (uint64_t)(U >= 'A' && U <= 'Z' ? U + 0x20 : U)) * Prime;
    };
    if (S == Scheme::Fields)
        for (const char C : Name) Step(C);
    else if (S == Scheme::SaltFirst)
    {
        if (Name.empty()) return 0;
        Step(Name[0]);
        for (const char C : Salt) Step(C);
        for (const char C : Name.substr(1)) Step(C);
    }
    else
    {
        for (const char C : Name) Step(C);
        for (const char C : Salt) Step(C);
    }
    return H;
}

std::string Name(uint64_t Key, Scheme S)
{
    static std::once_flag Once;
    static std::unordered_map<uint64_t, const char*> Known[3];
    std::call_once(Once, [] {
        for (const char* N : Names)
            for (const Scheme Each : { Scheme::Fields, Scheme::SaltFirst, Scheme::SaltAfter })
                Known[(int)Each].emplace(Hash(N, Each), N);
    });
    const auto& Table = Known[(int)S];
    if (const auto It = Table.find(Key); It != Table.end()) return It->second;
    return std::format("key_{:x}", Key);
}
}
