#pragma once

// Treyarch's sound name hashes (Black Ops to Black Ops III): an alias, a duck, a context type or
// value, a futz patch is stored as SND_HashName of its name (sdbm-like, lower case; OAT's
// CommonT6.h). SoundNames turns a hash back into its name from the names it was given: the
// zone's own (aliases, ducks) and the ones a game knows (its mod tools' globals and contexts).

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Treyarch
{
    constexpr uint32_t SoundHash(std::string_view Name)
    {
        if (Name.empty()) return 0;
        uint32_t R = 0x1505;
        for (char C : Name) R = (uint32_t)((C >= 'A' && C <= 'Z') ? C + 32 : (unsigned char)C) + 0x1003F * R;
        return R == 0 ? 1 : R;
    }

    class SoundNames
    {
    public:
        void Add(const std::string& Name)
        {
            if (!Name.empty()) Names.emplace(SoundHash(Name), Name);
        }
        void Add(uint32_t Hash, const std::string& Name)
        {
            if (Hash != 0 && !Name.empty()) Names.emplace(Hash, Name);
        }
        template <size_t N>
        void Add(const char* const (&List)[N])
        {
            for (const char* S : List) Add(std::string(S));
        }
        // the name of Hash, "" for 0; Known false when the hash is set and no name has it
        std::string Of(uint32_t Hash, bool& Known) const
        {
            Known = true;
            if (Hash == 0) return std::string();
            const auto It = Names.find(Hash);
            if (It != Names.end()) return It->second;
            Known = false;
            return std::string();
        }

    private:
        std::unordered_map<uint32_t, std::string> Names;
    };
}
