#include "stdafx.h"

#include "Games/GameModule.h"
#include "Games/IW/IwffContainer.h"

#include <algorithm>

namespace
{
    // A section every Black Ops 7 header carries (41 962 files of cod25 and lpc) and no Black Ops 6
    // one does (10 413): how a loose file is told from Black Ops 6's, which shares header version 25
    constexpr uint32_t Bo7OnlySection = 0xd4cb9f1b;

    bool OwnHeader(const std::string& Path)
    {
        uint32_t Magic = 0;
        std::vector<uint32_t> Types;
        return IwffContainer::ReadSections(Path, 0xe0, Magic, Types) &&
               std::find(Types.begin(), Types.end(), Bo7OnlySection) != Types.end();
    }

    // Games/IW/Container25.cpp (shared with Black Ops 6) decompresses it (Oodle blocks, the .fp update patch applied).
    // Its install is Steam's "Call of Duty HQ" (cod.exe), the zones in cod25 and cod25\lpc.
    const GameModuleRegistration Registered{
        { "BO7", "Black Ops 7", "cod.exe", "IWffa100", 0x19, GameSupportStage::Container, "cod25", OwnHeader, "bo7" },
    };
}
