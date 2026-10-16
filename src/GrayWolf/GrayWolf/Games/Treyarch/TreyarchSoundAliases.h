#pragma once

// Black Ops and Black Ops II sound aliases (the zone's SndBank, 32-bit) as mod tools alias rows:
// the CSV's own lineage (BO3's columns grew from BO2's), so most fields carry over as they are.
// BO2 after OAT's SndBankDumperT6 (field meanings, flag bits, enum tables; GPL-3.0); BO1 after
// OAT's T5_Assets.h snd_alias_t, its flags measured (Games/T5/DiskGame.cpp).

#include "SoundAliasCsv.h"

#include <cstdint>
#include <functional>
#include <string>

namespace Treyarch
{
    // BO2: the SndBank at Bank. File: the file the sound kind wrote for an alias's asset file name,
    // relative to the sounds folder ("" when none).
    void ReadBo2SoundBank(const SoundAliasCsv::ZoneView& Z, int64_t Bank, const std::function<std::string(const std::string& Asset)>& File,
                          SoundAliasCsv::Rows& Out);

    // BO1: the SndBank at Bank. File: the file written for the SoundFile at a zone offset.
    void ReadBo1SoundBank(const SoundAliasCsv::ZoneView& Z, int64_t Bank, const std::function<std::string(int64_t SoundFile)>& File,
                          SoundAliasCsv::Rows& Out);
}
