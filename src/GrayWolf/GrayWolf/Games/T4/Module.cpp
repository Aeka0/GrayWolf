#include "stdafx.h"

#include "Games/GameModule.h"

// World at War: one zlib stream behind a 12-byte header (FastFile.InfinityWard, version 387),
// every zone of the Steam install read by the zonekit t4 tables (zonekit/profiles/t4).
namespace
{
    const GameModuleRegistration Registered{
        { "T4", "Call of Duty: World at War", "CoDWaW.exe", "IWffu100", 387, GameSupportStage::List },
    };
}
