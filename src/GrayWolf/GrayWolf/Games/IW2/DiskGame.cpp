#include "stdafx.h"

// Call of Duty 2 on the disk template: its .iwd archives read by the CoD1/CoD2 family
// (Games/IW1IW2). Models name material files (materials/), which name .iwi images.

#include "Games/IW1IW2/IwDiskGame.h"

namespace
{
    // an archive by the maps it holds: the campaign's by their introscreen title (INTROSCREEN_
    // <LEVEL>_TITLE, decoytown "Hold the Line"), multiplayer's by mp/CoD2maps.arena's longname
    // (mp_farmhouse "Beltot, France")
    const DiskGame::IDiskGame::LevelNaming Levels = [] {
        DiskGame::IDiskGame::LevelNaming N;
        N.From = { "localized_*", "iw_*" };
        N.Keys = { { "*", { "INTROSCREEN_{NAME}_TITLE", "INTROSCREEN_{NAME}_PLACE" } } };
        N.Lists = { "mp/cod2maps.arena" };
        N.ByLevels = true;
        return N;
    }();
    const IW12::ArchiveGame Game({ "cod2", "Call of Duty 2", "IW2", ".iwd", false, &Levels });
    const DiskGame::Registration Registered(Game);
}
