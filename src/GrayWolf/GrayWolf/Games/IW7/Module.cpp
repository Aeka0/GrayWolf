#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // FastFile.InfinityWard.cpp decompresses both magics (the blocked LZ4 container, version
        // 0x653); the zones are walked with the zonekit "iw7" tables and exported by DiskGame.cpp.
        { "IW7", "Infinite Warfare",     "iw7_ship.exe",        "IWff0100", 1619, GameSupportStage::Assets },
        { "IW7", "Infinite Warfare",     "iw7_ship.exe",        "IWffu100", 1619, GameSupportStage::Assets },
    };
}
