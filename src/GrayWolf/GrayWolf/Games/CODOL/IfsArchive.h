#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <regex>
#include <string>
#include <vector>

// Tencent IIPS archives (`.ifs`, magic "nifs"), an MPQ v4 clone: HET/BET tables encrypted with
// MPQ's block cipher, names hashed with Jenkins' hashlittle2 and listed by a "(listfile)". The
// game files themselves are stored as they are; the listfile is sectored, encrypted and zlib'd.
namespace Ifs
{
    // The install's paths are Korean: kept as UTF-8 in a std::string, whatever the code page
    inline std::filesystem::path Utf8Path(const std::string& S)
    {
        return std::filesystem::path(std::u8string(S.begin(), S.end()));
    }
    inline std::string Utf8(const std::filesystem::path& P)
    {
        const std::u8string U = P.u8string();
        return std::string(U.begin(), U.end());
    }

    struct Entry
    {
        std::string Name;          // as the listfile spells it
        uint64_t Pos = 0;
        uint64_t Size = 0;
        uint64_t StoredSize = 0;
        uint32_t Flags = 0;
    };

    class Archive
    {
    public:
        bool Open(const std::string& FilePath, std::string& Error);
        const std::string& Path() const { return File; }
        // the named files, listfile and directories left out
        const std::vector<Entry>& Files() const { return Named; }
        bool Read(const Entry& E, std::vector<uint8_t>& Out, std::string& Error) const;

    private:
        bool Find(const std::string& Name, Entry& Out) const;

        std::string File;
        uint32_t HeaderSize = 0;
        uint32_t SectorSize = 0;
        uint64_t AndMask = 0, OrMask = 0;
        std::map<uint64_t, Entry> ByHash;
        std::vector<Entry> Named;
    };

    // Every archive of an install (<install>\IIPS\IIPSDownload\*.ifs), each file at its latest
    // version: a patch archive <stem>_V<n>.<k>.ifs replaces the files of <stem>_V<n>.ifs.
    class Install
    {
    public:
        bool Open(const std::string& InstallDir, std::string& Error);
        struct Located
        {
            size_t Archive = 0;
            Entry File;
            std::vector<uint32_t> Version;
        };
        // keyed by the lower-case path with '/' separators ("client_shipretail/chinese/x.ff")
        const std::map<std::string, Located>& Files() const { return Latest; }
        const Located* Find(const std::string& Key) const;
        bool Read(const Located& L, std::vector<uint8_t>& Out, std::string& Error) const;
        size_t ArchiveCount() const { return Archives.size(); }
        // the install folder an input names: the folder itself, or one holding IIPS\IIPSDownload
        static std::string ArchiveFolder(const std::string& InstallDir);
        static std::string Key(const std::string& Name);

    private:
        std::vector<Archive> Archives;
        std::map<std::string, Located> Latest;
    };

    // A file name pattern ('*' any run, '?' one character), case ignored
    std::regex GlobRegex(const std::string& Glob);
}
