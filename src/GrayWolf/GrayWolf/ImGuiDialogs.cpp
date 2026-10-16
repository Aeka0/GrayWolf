#include "stdafx.h"

#include "ImGuiDialogs.h"
#include "FileSystems.h"
#include "Strings.h"

#include <algorithm>
#include <cstring>
#include <filesystem>


void SetBuf(char* Buf, size_t Size, const std::string& Value)
{
    strncpy_s(Buf, Size, Value.c_str(), _TRUNCATE);
}

std::string AbsoluteFolder(const std::string& Folder)
{
    std::filesystem::path P(Folder);
    if (!Folder.empty() && P.is_relative()) P = std::filesystem::path(FileSystems::GetApplicationPath()) / P;
    return P.lexically_normal().string();
}

bool ContainsNoCase(const std::string& Haystack, const char* Needle)
{
    if (Needle == nullptr || *Needle == 0) return true;
    const auto It = std::search(Haystack.begin(), Haystack.end(), Needle, Needle + strlen(Needle),
        [](char A, char B) { return tolower((unsigned char)A) == tolower((unsigned char)B); });
    return It != Haystack.end();
}
