#include "stdafx.h"

// Modern Warfare (2019)'s fast file container, "IWffa100" version 11, decoded by the family's
// one decoder (Games/IW/IwffContainer.h).
//
// .ff: a 0x88-byte header (the zone size at 0x20), an authentication chunk ("IWffs100") of
// 0x8000 bytes, then the stream: "\x02IWC", u32 zone size, u32 flags, then blocks of
// { u32 compressed, u32 decompressed, u32 0 } + Oodle bytes padded to 4. A 0x4000-byte hash
// block is spliced in after every 0x800000 bytes of stream (Infinite Warfare's spacing).
// .fp: "IWffd100", a 0x38-byte header (the patch stream's size at 0x30), the base zone's
// header, the patched zone's header, its own authentication chunk, then a stream of the same
// shape whose content is VCDIFF with the whole base zone as source (2 MB target windows).

#include "Games/IW/IwffContainer.h"

namespace
{
    const IwffContainer::Registration Registered([] {
        IwffContainer::Layout L;
        L.HeaderVersion = 11;
        L.HeaderSize = 0x88;
        L.ZoneSizeAt = 0x20;
        L.AuthBeforeStream = 0x8000;
        L.HashEveryBytes = 0x800000;
        // The game ships oo2core_7_win64.dll; oo2core_8 decodes the same blocks
        L.OodleDlls = { L"oo2core_8_win64.dll", L"oo2core_7_win64.dll" };
        L.Title = "Modern Warfare blocks";
        return L;
    }());
}
