#include "stdafx.h"

#include "Games/IW3/Iw3ZoneFormat.h"

// World at War (T4): CoD4's layout with 7 blocks, so an XFile of 0x24
// (size, external size, 7 block sizes), then the IW3-family list and stream (Iw3ZoneFormat.h).
namespace
{
    const Iw3Family::ZoneFormat Format("t4", 0x24);
    const ZoneTables::ZoneFormatRegistration Registered(Format);
}
