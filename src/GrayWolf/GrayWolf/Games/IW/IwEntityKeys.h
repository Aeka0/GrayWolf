#pragma once

// The 64-bit key hashes of the entity strings of Black Ops 6 and 7 and Modern Warfare III (a map's
// GfxEntityPairs keys, as the game's spawn-var parser hashes a key's text), and their names. Each
// scheme is FNV-1a 64 of the lowercase name with prime 0x10000000233 (the name list:
// IwEntityKeyNames.h).

#include <cstdint>
#include <string>
#include <string_view>

namespace IwEntityKeys
{
    enum class Scheme
    {
        Fields,      // Modern Warfare III: basis 0x79d6530b0bb9b5d1
        SaltFirst,   // Black Ops 6 multiplayer and zombies, Black Ops 7: basis 0x1c2f2e3c8a257d07, the
                     // first character, then the salt "zt@f3yp(d[kkd=_@", then the rest
        SaltAfter,   // Black Ops 6 campaign: the same basis, the whole name, then the salt
    };
    uint64_t Hash(std::string_view Name, Scheme S);
    // The key's name, else "key_<hex>"
    std::string Name(uint64_t Key, Scheme S);
}
