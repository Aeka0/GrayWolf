#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // Same container family as Advanced Warfare (FastFile.Sledgehammer.cpp), told apart by
        // version. Models, animations, images and sounds read from disk (Games/S2/DiskGame.cpp).
        { "S2",  "WWII",                 "s2_sp64_ship.exe",    "S1ffu100", 5,    GameSupportStage::Assets },
        { "S2",  "WWII",                 "s2_sp64_ship.exe",    "S1ff0100", 5,    GameSupportStage::Assets },
    };
}
