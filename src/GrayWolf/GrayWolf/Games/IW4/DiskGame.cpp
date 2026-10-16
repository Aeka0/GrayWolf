#include "stdafx.h"

// Modern Warfare 2 (IW4, the 64-bit rebuild) on the disk template: the iw4 zonekit tables, the
// 64-bit readers (Games/IW4/IwDiskZone.h).

#include "Games/IW4/IwDiskZone.h"

namespace
{
    // the campaign by MENU_SP_SP_ (airport "Pas de russe"), Special Ops by SPECIAL_OPS_, the
    // multiplayer maps by MPUI_ (mp_rust "Rust") from the signed localized_code_post_gfx_mp, else
    // by the longname the map list mp/basemaps.arena gives them (mp_brecourt MPUI_WASTELAND)
    const DiskGame::IDiskGame::LevelNaming Levels{ { "code_post_gfx", "common", "code_post_gfx_mp", "localized_code_post_gfx_mp" },
                                                   { { "so_*", { "SPECIAL_OPS_{NAME}" } },
                                                     { "mp_*", { "MPUI_{REST}" } },
                                                     { "*", { "MENU_SP_SP_{NAME}" } } },
                                                   { "mp/basemaps.arena" } };
    const Iw64::GameInfo Info{ "mw2", "Call of Duty: Modern Warfare 2", "IW4", "iw4",
                               "IW4", &Iw64::Mw2, Scripts::Vm{}, &Levels };
    const Iw64::IwDiskGame Game(Info);
    const DiskGame::Registration Registered(Game);
}
