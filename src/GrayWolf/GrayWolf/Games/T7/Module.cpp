#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // Walked by the zonekit profile t7 (Games/T7/ZoneFormat.cpp), exported by disk-export --game bo3
        { "T7",  "Black Ops 3",          "BlackOps3.exe",       "TAff0000", 0,    GameSupportStage::Assets },
    };
}
