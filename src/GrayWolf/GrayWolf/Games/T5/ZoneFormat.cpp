#include "stdafx.h"

#include "Games/IW3/Iw3ZoneFormat.h"

// Call of Duty: Black Ops (T5): World at War's layout, an XFile of 0x24
// (size, external size, 7 block sizes), then the IW3-family list and stream (Iw3ZoneFormat.h); block
// 4 is current for the list.
namespace
{
    const Iw3Family::ZoneFormat Format("t5", 0x24);
    const ZoneTables::ZoneFormatRegistration Registered(Format);
}
