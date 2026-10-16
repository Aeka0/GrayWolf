#include "stdafx.h"

#include "Games/GameModule.h"

namespace
{
    const GameModuleRegistration Registered{
        // Same container family as Advanced Warfare (FastFile.Sledgehammer.cpp), told apart by its
        // version 0x42. Models, animations, images, sounds and maps read from disk (Games/H1/DiskGame.cpp,
        // disk-export --game mwr).
        { "H1",  "Modern Warfare Remastered", "h1_mp64_ship.exe", "S1ffu100", 0x42, GameSupportStage::Assets },
        { "H1",  "Modern Warfare Remastered", "h1_mp64_ship.exe", "S1ff0100", 0x42, GameSupportStage::Assets },
    };
}
