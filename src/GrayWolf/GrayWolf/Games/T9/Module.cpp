#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // zonekit profile t9: every zone read by table (zone-walk --profile t9); no export yet.
        // The magic is "TAFF" and the version its header's version section (FastFile::Identify).
        { "T9",  "Black Ops Cold War",   "BlackOpsColdWar.exe", "TAFF", 0x64, GameSupportStage::List },
    };
}
