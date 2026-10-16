#include "stdafx.h"

#include "Games/IW3/Iw3ZoneFormat.h"

// Call of Duty 4 (IW3): XFile 0x2c (size, external size, 9 block sizes),
// then the IW3-family list and stream (Iw3ZoneFormat.h).
namespace
{
    const Iw3Family::ZoneFormat Format("iw3", 0x2c);
    const ZoneTables::ZoneFormatRegistration Registered(Format);
}
