#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // Read from disk with the zonekit "s1" tables (Games/S1/DiskGame.cpp): models with their
        // textures, animations, images, sounds and maps. WWII names its version; Advanced
        // Warfare (version 0x72e) is the magic-only match for the rest, signed zones included.
        { "S1",  "Advanced Warfare",     "s1_sp64_ship.exe",    "S1ffu100", 0,    GameSupportStage::Assets },
        { "S1",  "Advanced Warfare",     "s1_sp64_ship.exe",    "S1ff0100", 0,    GameSupportStage::Assets },
    };
}
