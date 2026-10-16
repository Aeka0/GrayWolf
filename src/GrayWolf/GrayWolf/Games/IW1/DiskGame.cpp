#include "stdafx.h"

// Call of Duty (2003) on the disk template: its .pk3 archives read by the CoD1/CoD2 family
// (Games/IW1IW2). Models name their colour maps directly (skins/, textures/).

#include "Games/IW1IW2/IwDiskGame.h"

namespace
{
    // an archive by the maps it holds: the campaign's by their introscreen place (INTROSCREEN_
    // <LEVEL>_PLACE, pavlov "Stalingrad, Russia"), multiplayer's by file name (mp/CoDmaps.arena's
    // longname is the file name)
    const DiskGame::IDiskGame::LevelNaming Levels = [] {
        DiskGame::IDiskGame::LevelNaming N;
        N.From = { "localized_*", "pak*" };
        N.Keys = { { "*", { "INTROSCREEN_{NAME}_PLACE" } } };
        N.Lists = { "mp/codmaps.arena" };
        N.ByLevels = true;
        return N;
    }();
    const IW12::ArchiveGame Game({ "cod1", "Call of Duty", "IW1", ".pk3", true, &Levels });
    const DiskGame::Registration Registered(Game);
}
