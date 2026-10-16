#include "stdafx.h"

#include "Games/GameModule.h"

// Call of Duty 4: one zlib stream behind a 12-byte header (FastFile.InfinityWard), every zone of
// the Steam install read by the zonekit iw3 tables (zonekit/profiles/iw3).
namespace
{
    const GameModuleRegistration Registered{
        { "IW3", "Call of Duty 4: Modern Warfare", "iw3mp.exe", "IWffu100", 5, GameSupportStage::List },
    };
}
