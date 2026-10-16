#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // Models, animations, images and sounds read from disk. Ghosts ships each zone in one of two
        // magics and the signed one has to be claimed too, or the signed zones fall through to the
        // container-only path.
        { "IW6", "Ghosts",               "iw6sp64_ship.exe",    "IWffu100", 565,  GameSupportStage::Assets },
        { "IW6", "Ghosts",               "iw6sp64_ship.exe",    "IWff0100", 565,  GameSupportStage::Assets },
    };
}
