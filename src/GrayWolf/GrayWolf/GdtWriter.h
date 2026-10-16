#pragma once

// The text of a Mod Tools GDT (Black Ops III's APE, World at War's and Black Ops' asset manager):
// a brace-wrapped list of entries, each `"name" ( "kind.gdf" )` and its quoted key/value lines,
// tab-indented. APE writes CRLF; a writer that has always written LF passes "\n".

#include <string>
#include <utility>
#include <vector>

namespace Gdt
{
    using Fields = std::vector<std::pair<std::string, std::string>>;

    // One entry, its fields in the order given
    std::string Entry(const std::string& Name, const std::string& Gdf, const Fields& Values, const char* Eol = "\r\n");
    // A .gdt file of entries
    std::string Document(const std::vector<std::string>& Entries, const char* Eol = "\r\n");
    // A file path as a GDT value names it: each separator ('/' or '\') as the escaped "\\" the
    // Mod Tools write
    std::string Path(const std::string& File);
}
