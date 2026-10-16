#include "stdafx.h"

#include "Games/IW/IWMaterialBooks.h"

#include "ExportNaming.h"
#include "ExportWriters.h"
#include "FastFile.h"
#include "ZonekitZone.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <map>
#include <mutex>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

namespace IWMaterialBooks
{
    namespace
    {
        bool ReadZoneMaterials(const fs::path& Ff, const ExportWriters::LoadedProfile& Loaded, const Layout& L, Book& Out)
        {
            std::vector<uint8_t> Z;
            std::string Error;
            if (!FastFile::Decompress(Ff.string(), Z, Error)) return false;
            const auto& P = Loaded.Profile;
            const auto R = ZoneTables::WalkZone(Z, Loaded.Tables, P.TypeNames, P.Rules, true);
            if (R.ParseFailed) return false;
            // where each pointer field points once relocated (the last link of a field, as served)
            std::unordered_map<int64_t, int64_t> Target;
            for (const auto& [Field, To] : ResolveLinks(R, P.Rules, true, &Z).Links) Target[Field] = To;
            const auto Ptr = [&](int64_t Field) {
                const auto It = Target.find(Field);
                return It == Target.end() ? (int64_t)-1 : It->second;
            };
            const auto Str = [&](int64_t Field) {
                std::string S;
                for (int64_t K = Ptr(Field); K >= 0 && K < (int64_t)Z.size() && Z[(size_t)K]; K++) S.push_back((char)Z[(size_t)K]);
                return S;
            };
            const auto Unprefixed = [](std::string S) { return !S.empty() && S[0] == ',' ? S.substr(1) : S; };
            for (const auto& I : R.Instances)
            {
                if (I.Owner >= R.OwnerNames.size() || R.OwnerNames[I.Owner] != "material") continue;
                const int64_t Table = Ptr(I.Pos + L.Images);
                const uint8_t Count = ExportNaming::At<uint8_t>(Z, I.Pos + L.ImageCount);
                std::string Name = Str(I.Pos);
                if (L.WithSlotsOnly)
                {
                    if (Count == 0 || Table < 0) continue;
                    Name = Unprefixed(Name);
                }
                else if (!Name.empty() && Name[0] == ',')
                    continue;
                if (Name.empty() || Out.count(Name)) continue;
                ImageList Images;
                for (int64_t K = 0; Table >= 0 && K < Count; K++)
                {
                    const int64_t Slot = Table + K * L.SlotSize;
                    const int64_t Img = Ptr(Slot + L.SlotImage);
                    std::string N = Img < 0 ? std::string() : Unprefixed(Str(Img + L.ImageName));
                    if (!N.empty()) Images.emplace_back(ExportNaming::At<uint32_t>(Z, Slot), std::move(N));
                }
                Out.emplace(std::move(Name), std::move(Images));
            }
            return true;
        }
    }

    // A line per material, "name<TAB>semantic<TAB>image..." (semantic in hex)
    std::shared_ptr<const Book> ZoneBook(const fs::path& Ff, const std::string& CacheDir, const Layout& L)
    {
        static std::mutex Lock;
        static std::map<std::string, std::shared_ptr<const Book>> Read;
        std::lock_guard<std::mutex> Guard(Lock);
        std::string Error;
        const auto* Loaded = ExportWriters::ZoneProfile(L.Profile, Error);
        std::error_code Ec;
        const auto Size = fs::file_size(Ff, Ec);
        if (Loaded == nullptr || Ec) return nullptr;
        const auto Time = fs::last_write_time(Ff, Ec).time_since_epoch().count();
        const fs::path File = fs::path(CacheDir) / std::format("{}_{:x}_{:x}_{}.tsv", Ff.stem().string().c_str(), (unsigned long long)Size,
                                                                   (unsigned long long)Time, ProfileKey(*Loaded).c_str());
        if (const auto It = Read.find(File.string()); It != Read.end()) return It->second;
        auto Out = std::make_shared<Book>();
        if (std::ifstream In(File); In)
        {
            for (std::string Line; std::getline(In, Line);)
            {
                std::istringstream Fields(Line);
                std::string Name, Sem, Img;
                if (!std::getline(Fields, Name, '\t')) continue;
                ImageList Images;
                while (std::getline(Fields, Sem, '\t') && std::getline(Fields, Img, '\t'))
                    Images.emplace_back((uint32_t)std::stoul(Sem, nullptr, 16), Img);
                Out->emplace(Name, std::move(Images));
            }
            return Read[File.string()] = Out;
        }
        if (!ReadZoneMaterials(Ff, *Loaded, L, *Out)) return nullptr;
        // written aside then renamed: another process may read the same book meanwhile
        fs::create_directories(CacheDir, Ec);
        if (std::ofstream O(File.string() + ".tmp", std::ios::binary); O)
        {
            for (const auto& [Name, Images] : *Out)
            {
                O << Name;
                for (const auto& [Sem, Img] : Images) O << '\t' << std::format("{:x}", Sem) << '\t' << Img;
                O << '\n';
            }
            O.close();
            fs::rename(File.string() + ".tmp", File, Ec);
        }
        return Read[File.string()] = Out;
    }

    std::vector<fs::path> ZonesMatching(const fs::path& Dir, const std::string& Pattern)
    {
        const std::regex Match(Pattern, std::regex::icase);
        std::vector<fs::path> Zones;
        std::error_code Ec;
        for (const auto& E : fs::directory_iterator(Dir, Ec))
            if (E.is_regular_file() && std::regex_match(E.path().filename().string(), Match)) Zones.push_back(E.path());
        std::sort(Zones.begin(), Zones.end(), [](const fs::path& A, const fs::path& B) {
            const bool Pa = A.filename().string().rfind("patch_", 0) == 0, Pb = B.filename().string().rfind("patch_", 0) == 0;
            return Pa != Pb ? Pa : A.filename() < B.filename();
        });
        return Zones;
    }

    fs::path InstallDir(const std::string& FastFile, const char* Marker)
    {
        const fs::path Dir = fs::path(FastFile).parent_path();
        std::error_code Ec;
        if (!fs::exists(Dir / Marker, Ec) && fs::exists(Dir.parent_path() / Marker, Ec)) return Dir.parent_path();
        return Dir;
    }

    Lookup::Lookup(Layout L, std::string CacheDir, ZoneList Zones) : L(L), CacheDir(std::move(CacheDir)), Zones(std::move(Zones)) {}

    bool Lookup::Find(const std::string& Name, ImageList& Out)
    {
        if (!Read)
        {
            Read = true;
            for (const auto& F : Zones())
                if (auto B = ZoneBook(F, CacheDir, L)) Books.push_back(std::move(B));
        }
        for (const auto& B : Books)
            if (const auto It = B->find(Name); It != B->end())
            {
                Out = It->second;
                Booked.insert(Name);
                return true;
            }
        NotBooked.insert(Name);
        return false;
    }

    void Lookup::Measure(DiskGame::Totals& T, const char* Filled, const char* Unfilled) const
    {
        if (!Booked.empty()) T.Counters[Filled] += Booked.size();
        if (!NotBooked.empty()) T.Counters[Unfilled] += NotBooked.size();
    }
}
