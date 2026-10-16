#pragma once

// Zones loaded beside a served zone, as the game has them loaded at the same time. These are the
// global zones a model's mesh is in, or a map's transient zones. Each is walked with the same
// profile and its own pointers are resolved. It is placed above the served zone and served with it
// (ProcessReader::AddVirtual). The references every zone leaves unresolved are then linked across
// all of them, as the game's shared tables would link them: by key (stream.keyed_refs: the data an
// inline pointer filed) and by asset name hash (an instance of the type the reference reads).
// Black Ops 6 (Games/BO6/DiskGame.cpp) loads the zones the install index says hold its models'
// xmodelsurfs (DiskGame::ZoneIndex::PartIn); Black Ops 7 (Games/BO7/DiskGame.cpp) a map's global
// zones (loaded before it, Before) and its transient zones, as the game loads them.
// The keys: stream.keyed_refs' (Black Ops 6) or the rewind table's (stream.rewind_prepare, Black
// Ops 7: the walker links a zone's own, the rest is left to Link).

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ExportWriters.h"
#include "ZoneTableWalker.h"

namespace IwBeside
{
    struct Zone
    {
        std::string Name;
        std::vector<uint8_t> Bytes;
        ZoneTables::ZoneResult R;
        uint64_t Base = 0;   // where it is served
        bool Before = false; // loaded before the served zone (a level's global zones): its keys and
                             // assets only fill what the others do not define, its own unresolved
                             // references are counted apart
    };

    class Set
    {
    public:
        // Loads each fast file (FastFile::Decompress), walks it with Loaded and resolves its own
        // links (read on up to 4 threads). Each one is placed from Above upward (after the zones
        // already loaded), 16 MB aligned, in the order of Paths. Returns how many loaded; the others
        // are counted in Counters.
        size_t Load(const std::vector<std::string>& Paths, const ExportWriters::LoadedProfile& Loaded, uint64_t Above,
                    std::map<std::string, size_t>& Counters, bool Before = false);
        // A reference no zone defines: its zone ("" for Main), field (its offset in that zone), value, the asset
        // type it names ("asset" when unknown) or "key"
        using Unresolved = std::function<void(const std::string& Zone, uint64_t Field, uint64_t Value, const std::string& Type)>;
        // Links the references Main (served at MainBase) and the loaded zones left unresolved,
        // across all of them, Main's own instances and keys first (it loaded first), the Before
        // zones' last. Main's fields are written through PutMain. OnUnresolved hears every
        // reference left unresolved but the Before zones' own.
        void Link(const ZoneTables::ZoneResult& Main, uint64_t MainBase, const std::vector<uint8_t>& MainBytes,
                  const ZoneTables::StreamRules& Rules, const std::function<void(uint64_t Field, uint64_t Value)>& PutMain,
                  std::map<std::string, size_t>& Counters, const Unresolved& OnUnresolved = {});
        // Serves the loaded zones beside the current session's image (call again on a new session)
        void Serve() const;
        // The bytes at an address of a loaded zone, null when N bytes are not all in one
        const uint8_t* Locate(uint64_t Address, uint64_t N) const;
        // Writes 8 bytes at an address of a loaded zone; false when it is in none
        bool Put(uint64_t Field, uint64_t Value);
        bool Empty() const { return Zones.empty(); }
        uint64_t End() const { return Next; }
        const std::vector<std::unique_ptr<Zone>>& All() const { return Zones; }

    private:
        std::vector<std::unique_ptr<Zone>> Zones;
        uint64_t Next = 0;
    };
}
