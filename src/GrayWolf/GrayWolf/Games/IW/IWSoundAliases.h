#pragma once

// The Infinity Ward engine's sound aliases (snd_alias_list_t / snd_alias_t, CoD4 to MWR, and World
// at War's) as mod tools alias rows. One reader, each game giving its snd_alias_t layout (the
// offsets of the fields it has, -1 for the others: OAT's IW3/IW4/IW5/T4 headers, Joelrau's
// zonetool IW6/S1/H1 Structs.hpp, both GPL-3.0). Values convert to the CSV's units: linear
// volumes to dBSPL, pitch ratios to cents, distances rounded to inches. What has no BO3 column
// (chain alias, volmod, channel, master/slave, LFE) is left out.

#include "SoundAliasCsv.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace IWSoundAliases
{
    struct Layout
    {
        // snd_alias_list_t: name 0, head at ListHead (-1: the pointer width), the variant count
        int64_t ListHead = -1, ListCount = 0x10;
        int ListCountBytes = 4;
        // snd_alias_t
        int64_t Size = 0;
        int64_t Subtitle = -1, Secondary = -1, SoundFile = -1;
        int64_t VolMin = -1, VolMax = -1, PitchMin = -1, PitchMax = -1, DistMin = -1, DistMax = -1, DistReverbMax = -1;
        int64_t Flags = -1, Probability = -1, CenterPercentage = -1, StartDelay = -1;
        int64_t EnvelopMin = -1, EnvelopMax = -1, EnvelopPercentage = -1, AllowDoppler = -1;
        // World at War's own (floats; limit and randomize ints)
        int64_t MinPriority = -1, MaxPriority = -1, MinPriorityThreshold = -1, MaxPriorityThreshold = -1, ReverbSend = -1;
        int64_t OcclusionLevel = -1, LimitType = -1, EntityLimitCount = -1, RandomizeType = -1;
        int LoopingBit = 0;     // the flags' looping bit
        int LoopingBits = 1;    // ... and how many (Infinite Warfare: 3, any set loops)
        int Is3DBit = -1;       // the flags' 3D bit (spatialize), -1: none
        // Infinite Warfare: no SoundFile, the storage in the flags (FlagsType: its shift; 0 loaded,
        // 1 hybrid PCM, 2 streamed, 3 primed), 2D/3D by the channel's name (ChannelPan: '2', '3' or
        // '-' per channel, the channel at ChannelShift, 8 bits)
        int FlagsType = -1, ChannelShift = -1;
        const char* ChannelPan = nullptr;
        // SoundFile {u8 type 0, u8 exists 1, the union at the pointer width}: a primed sound's
        // LoadedSound pointer, from the SoundFile (Ghosts 0x20, AW/MWR 8); -1: no primed sounds
        int64_t PrimedLoaded = -1;
    };

    // The file the sound kind wrote for variant Index of alias list Name whose SoundFile is at
    // File (zone offset, -1 when not in the zone), relative to the sounds folder with its
    // extension; "" for none.
    using FileOf = std::function<std::string(const std::string& Name, uint32_t Index, int64_t File)>;

    // How the IW exporters (GameReaders' LoadedSound, StreamedSound, PrimedSound) name a variant's
    // file: a loaded sound by its LoadedSound's name without extension, a primed one by that name
    // with it, a streamed one <alias>_<index> (lower case), by the first alias naming that
    // SoundFile. Seen keeps the streamed ones met, across the zone's lists in their order.
    std::string LiveName(const SoundAliasCsv::ZoneView& Z, const Layout& L, const std::string& Name, uint32_t Index, int64_t File,
                         std::map<int64_t, std::string>& Seen);

    // Reads the snd_alias_list_t at List (Name: its name) into rows, the FileSpec FileOf's file
    // (backslashes).
    void Read(const SoundAliasCsv::ZoneView& Z, int64_t List, const std::string& Name, const Layout& L, const FileOf& File,
              SoundAliasCsv::Rows& Out);

    // Every list of Lists (position, name; in the zone's order, references left out), each variant's
    // file named by LiveName: the rows of a zone whose sounds IWPakSounds wrote
    void ReadLive(const SoundAliasCsv::ZoneView& Z, const std::vector<std::pair<int64_t, std::string>>& Lists, const Layout& L,
                  SoundAliasCsv::Rows& Out);
}
