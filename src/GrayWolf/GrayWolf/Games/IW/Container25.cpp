#include "stdafx.h"

// The fast file container of header version 25 ("IWffa100"): Black Ops 7 (cod25), and Black Ops 6
// by the same version, decoded by the family's one decoder (IwffContainer.h). The layout first
// from atian-cod-tools' decompressor_iw.cpp, each field checked on the install's files: a 0xe0 header, "TAFA" sections
// and a blob, then the signed stream; the stream's bytes (u32 0x14) end the file.

#include "IwffContainer.h"

namespace
{
    const IwffContainer::Registration Registered([] {
        IwffContainer::Layout L;
        L.HeaderVersion = 25;
        L.HeaderSize = 0xe0;
        L.ZoneSizeAt = 0x38;
        L.CheckZoneSize = true;
        L.XFileVersion = 1;
        L.Sections = true;
        return L;
    }());
}
