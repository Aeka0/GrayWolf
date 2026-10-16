#include "stdafx.h"

#include "Games/Treyarch/TreyarchZoneFormat.h"

// Black Ops 4 (T8): the Treyarch header, its default layout (0x20 bytes, count 0x10, list 0x18).
namespace
{
    const TreyarchZoneFormat Format("t8");
    const ZoneTables::ZoneFormatRegistration Registered(Format);
}
