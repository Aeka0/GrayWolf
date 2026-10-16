#include "stdafx.h"

#include "Games/Treyarch/TreyarchZoneFormat.h"

// Black Ops Cold War (T9): the Treyarch header, its layout from profiles/t9 "zone_header"
// (0x28 bytes, count 0x18, list 0x20: the string ids pointer sits between).
namespace
{
    const TreyarchZoneFormat Format("t9");
    const ZoneTables::ZoneFormatRegistration Registered(Format);
}
