#include "stdafx.h"

#include "GdtWriter.h"

namespace Gdt
{
std::string Entry(const std::string& Name, const std::string& Gdf, const Fields& Values, const char* Eol)
{
    std::string Out = "\t\"" + Name + "\" ( \"" + Gdf + "\" )" + Eol + "\t{" + Eol;
    for (const auto& [K, V] : Values) Out += "\t\t\"" + K + "\" \"" + V + "\"" + Eol;
    return Out + "\t}" + Eol;
}

std::string Document(const std::vector<std::string>& Entries, const char* Eol)
{
    std::string Out = std::string("{") + Eol;
    for (const auto& E : Entries) Out += E;
    return Out + "}" + Eol;
}

std::string Path(const std::string& File)
{
    std::string Out;
    Out.reserve(File.size() + 8);
    for (const char C : File)
        if (C == '/' || C == '\\') Out += "\\\\";
        else Out += C;
    return Out;
}
}
