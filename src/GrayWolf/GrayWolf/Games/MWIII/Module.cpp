#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // Games/MWIII/Container.cpp decompresses it (Oodle blocks, the .fp update patch applied).
        { "MWIII", "Modern Warfare III (2023)", "cod23-cod.exe", "IWffa100", 24, GameSupportStage::Container },
    };
}
