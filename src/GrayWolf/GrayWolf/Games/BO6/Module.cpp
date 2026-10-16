#include "stdafx.h"

#include "Games/GameModule.h"
#include "Games/IW/IwffContainer.h"

#include <algorithm>

namespace
{
    constexpr uint32_t SectionsSigned = 0x41464154; // "TAFA"
    constexpr uint32_t SectionsPlain = 0x46464154;  // "TAFF"
    // A section only Black Ops 7's headers carry (41 962 of its files, none of Black Ops 6's 10 413)
    constexpr uint32_t Bo7OnlySection = 0xd4cb9f1b;

    // A loose file by its header: cod24's are signed ("TAFA", all 10 013), sp24's are not ("TAFF",
    // all 400; the campaign's exe is another build, read by its own profile)
    bool Multiplayer(const std::string& Path)
    {
        uint32_t Magic = 0;
        std::vector<uint32_t> Types;
        return IwffContainer::ReadSections(Path, 0xe0, Magic, Types) && Magic == SectionsSigned &&
               std::find(Types.begin(), Types.end(), Bo7OnlySection) == Types.end();
    }
    bool Campaign(const std::string& Path)
    {
        uint32_t Magic = 0;
        std::vector<uint32_t> Types;
        return IwffContainer::ReadSections(Path, 0xe0, Magic, Types) && Magic == SectionsPlain &&
               std::find(Types.begin(), Types.end(), Bo7OnlySection) == Types.end();
    }

    // Games/IW/Container25.cpp (shared with Black Ops 7) decompresses it (Oodle blocks, the .fp update patch applied);
    // each row's profile reads and exports (DiskGame.cpp: disk ids bo6 and bo6sp).
    const GameModuleRegistration Registered{
        { "BO6", "Black Ops 6", "cod24-cod.exe", "IWffa100", 0x19, GameSupportStage::Assets, "cod24", Multiplayer, "bo6" },
        { "BO6SP", "Black Ops 6 (campaign)", "sp24-cod.exe", "IWffa100", 0x19, GameSupportStage::Assets, "sp24", Campaign, "bo6sp" },
    };
}
