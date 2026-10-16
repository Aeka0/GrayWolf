#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // bo4-disk-export: images, models, anims and sounds for the whole install.
        { "T8",  "Black Ops 4",          "BlackOps4.exe",       "TAff0000", 0x27F, GameSupportStage::Assets },
    };
}
