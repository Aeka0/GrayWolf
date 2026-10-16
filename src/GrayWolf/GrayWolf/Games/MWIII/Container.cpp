#include "stdafx.h"

// Modern Warfare III (2023)'s fast file, "IWffa100" version 24, and its .fp update patch, decoded
// by the family's one decoder (Games/IW/IwffContainer.h): a 0xe0-byte header (zone size at 0x38),
// a "TAFF" section list, "\x02IWC" and its 0x8000-byte "IWffs100" chunk, then the blocks with a
// 0x4000-byte hash block before every 0x200th.

#include "Games/IW/IwffContainer.h"

namespace
{
    const IwffContainer::Registration Registered([] {
        IwffContainer::Layout L;
        L.HeaderVersion = 24;
        L.HeaderSize = 0xe0;
        L.ZoneSizeAt = 0x38;
        L.CheckZoneSize = true;
        L.Sections = true;
        L.Title = "Modern Warfare III blocks";
        return L;
    }());
}
