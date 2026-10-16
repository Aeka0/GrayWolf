#include "stdafx.h"

// Modern Warfare 3 (IW5, the 64-bit rebuild) on the disk template: the iw5 zonekit tables, MW2's
// 64-bit readers with MW3's material layout (Games/IW4/IwDiskZone.h).

#include "Games/IW4/IwDiskZone.h"

namespace
{
    // the rich presence strings: PRESENCE_MP_SEATOWN, PRESENCE_SP_DUBAI; Survival by its map's
    // SPECIAL_OPS_SURVIVAL_
    const DiskGame::IDiskGame::LevelNaming Levels{ { "code_post_gfx", "common" },
                                                   { { "so_survival_mp_*", { "SPECIAL_OPS_SURVIVAL_{REST}" } },
                                                     { "*", { "PRESENCE_{NAME}", "PRESENCE_SP_{NAME}", "MENU_SP_{NAME}" } } } };
    const Iw64::GameInfo Info{ "mw3", "Call of Duty: Modern Warfare 3", "IW5", "iw5",
                               "IW5", &Iw64::Mw3,
                               Scripts::GscToolBin("iw5", "pc64"), &Levels };
    const Iw64::IwDiskGame Game(Info);
    const DiskGame::Registration Registered(Game);
}
