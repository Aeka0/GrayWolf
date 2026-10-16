#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // Games/IW8/Container.cpp decompresses it (Oodle blocks, the .fp update patch applied).
        { "IW8", "Modern Warfare (2019)", "ModernWarfare.exe", "IWffa100", 11, GameSupportStage::Assets },
    };
}
