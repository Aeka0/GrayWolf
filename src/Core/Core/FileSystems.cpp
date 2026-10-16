#include "stdafx.h"

#include "FileSystems.h"

#include "Strings.h"

#include <Windows.h>

namespace
{
    // Empty or ASCII blanks only: a name of nothing
    bool Blank(const std::string& Value)
    {
        return Value.find_first_not_of(" \t\n\v\f\r") == std::string::npos;
    }
}

std::string FileSystems::GetFileName(const std::string& Path)
{
    if (Blank(Path)) { return ""; }

    const auto Separator = Path.find_last_of("\\/:");
    return (Separator == std::string::npos) ? Path : Path.substr(Separator + 1);
}

std::string FileSystems::GetFileNameWithoutExtension(const std::string& Path)
{
    if (Blank(Path)) { return ""; }

    const auto Name = GetFileName(Path);
    return Name.substr(0, Name.find_last_of('.'));
}

std::string FileSystems::GetFileNamePurgeExtensions(const std::string& Path)
{
    if (Blank(Path)) { return ""; }

    const auto Name = GetFileName(Path);
    return Name.substr(0, Name.find_first_of('.'));
}

std::string FileSystems::GetExtension(const std::string& Path)
{
    if (Blank(Path)) { return ""; }

    const auto Dot = Path.find_last_of(".\\/:");
    if (Dot == std::string::npos || Path[Dot] != '.' || Dot == Path.size() - 1) { return ""; }
    return Path.substr(Dot);
}

std::string FileSystems::GetDirectoryName(const std::string& Path)
{
    if (Blank(Path)) { return ""; }

    const auto IsSeparator = [](char Char) { return Char == '\\' || Char == '/'; };
    // "C:\" is a root: what is under it keeps it
    const size_t RootLength = (Path.size() > 2 && Path[1] == ':' && IsSeparator(Path[2])) ? 3 : 0;
    if (Path.size() <= RootLength) { return ""; }

    size_t End = Path.size();
    while (End > RootLength && !IsSeparator(Path[--End]));
    return Path.substr(0, End);
}

std::string FileSystems::GetApplicationPath()
{
    wchar_t Buffer[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, Buffer, MAX_PATH);
    return GetDirectoryName(Strings::ToNormalString(Buffer));
}
