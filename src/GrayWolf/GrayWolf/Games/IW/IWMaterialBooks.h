#pragma once

#include "DiskGame.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// What a name-only material header (",name", no image slot) stands for, in the Infinity Ward
// engine's titles: the game loads other zones first (the common zones, a level's techsets_ zone)
// that define the materials a level's, a weapon's or a transient zone's headers name. A book is
// one such zone's materials and their images (semantic hash, image name), read off its walk once
// and kept on disk keyed by the profile and the fast file's size and time.
namespace IWMaterialBooks
{
    using ImageList = std::vector<std::pair<uint32_t, std::string>>;
    using Book = std::unordered_map<std::string, ImageList>;

    // Where a game's Material keeps its images: a u8 count and a table of {u32 semantic, pad,
    // image*} slots; and where its GfxImage keeps its name.
    struct Layout
    {
        const char* Profile = nullptr;
        int64_t ImageCount = 0, Images = 0, SlotSize = 0x10, SlotImage = 8;
        int64_t ImageName = 0;
        // Only a material with image slots is booked, named without its ',' (otherwise every
        // material named in place is, with or without slots, and a ",name" header never)
        bool WithSlotsOnly = false;
    };

    // The fast file's book, nullptr when it does not read. CacheDir keeps the books across runs.
    std::shared_ptr<const Book> ZoneBook(const std::filesystem::path& FastFile, const std::string& CacheDir, const Layout& L);

    // The fast files in Dir whose names match Pattern (case ignored), patch_ zones first: a
    // material the patch redefines is the patch's
    std::vector<std::filesystem::path> ZonesMatching(const std::filesystem::path& Dir, const std::string& Pattern);

    // The folder of a zone's install: its own, or one up for a localized zone (french/fra_*.ff),
    // the one holding Marker
    std::filesystem::path InstallDir(const std::string& FastFile, const char* Marker);

    // A zone's material headers looked up in the books Zones lists, in that order, the books
    // read on the first header met
    class Lookup
    {
    public:
        using ZoneList = std::function<std::vector<std::filesystem::path>()>;
        Lookup(Layout L, std::string CacheDir, ZoneList Zones);

        bool Find(const std::string& Name, ImageList& Out);
        // How many headers a book filled (Filled) and how many none did (Unfilled)
        void Measure(DiskGame::Totals& T, const char* Filled, const char* Unfilled) const;

    private:
        Layout L;
        std::string CacheDir;
        ZoneList Zones;
        bool Read = false;
        std::vector<std::shared_ptr<const Book>> Books;
        std::set<std::string> Booked, NotBooked;
    };
}
