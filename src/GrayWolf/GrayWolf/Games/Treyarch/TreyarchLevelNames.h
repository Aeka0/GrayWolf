#pragma once

// Black Ops 4 and Cold War keep a localized string's key as its hash: the 60-bit FNV-1a of the
// lower-case key ("mpui/seaside"), which their LocalizeEntry names localize_<hex> when no name
// index knows it. A level name rule's keys are looked up that way (LevelNaming::Hashed).

#include "ExportNaming.h"

#include <format>
#include <string>

namespace Treyarch
{
    inline std::string HashedLocalizeKey(const std::string& Key)
    {
        const uint64_t Hash = ExportNaming::Fnv1a(ExportNaming::Lower(Key)) & 0x0FFFFFFFFFFFFFFFull;
        return std::format("localize_{:x}", (unsigned long long)Hash);
    }
}
