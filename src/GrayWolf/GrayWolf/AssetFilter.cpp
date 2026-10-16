#include "stdafx.h"

#include "AssetFilter.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <algorithm>
#include <cctype>
#include <sstream>

static std::string ToLower(std::string Value)
{
    for (auto& C : Value) C = (char)std::tolower((unsigned char)C);
    return Value;
}

static bool Contains(const std::string& Haystack, const std::string& Needle)
{
    return ToLower(Haystack).find(ToLower(Needle)) != std::string::npos;
}

// "Everything"-style name query: whitespace-separated terms are ANDed; a term prefixed with '!' must
// NOT appear. Case-insensitive. An empty query (or one made only of lone '!') matches everything.
// e.g. "mike4 !rec" = name contains "mike4" AND does not contain "rec".
static bool MatchNameQuery(const std::string& Query, const std::string& Name)
{
    std::istringstream Ss(Query);
    std::string Term;
    while (Ss >> Term)
    {
        bool Negate = false;
        if (Term.front() == '!') { Negate = true; Term.erase(Term.begin()); }
        if (Term.empty()) continue; // a lone '!' is ignored

        const bool Has = Contains(Name, Term);
        if (Negate ? Has : !Has)
            return false;
    }
    return true;
}

bool MatchesFilter(const AssetFilter& Filter, const AssetEntry& Entry)
{
    if (Filter.Kind && *Filter.Kind != Entry.Kind)
        return false;

    // Image-specific predicates. If any is set, the entry must be an image to satisfy it.
    const bool WantsImageMeta =
        Filter.MinWidth || Filter.MaxWidth || Filter.MinHeight || Filter.MaxHeight ||
        Filter.MinDim || Filter.MaxDim || Filter.HasAlpha || Filter.Cubemap || !Filter.Formats.empty();

    if (WantsImageMeta)
    {
        if (Entry.Kind != AssetKind::Image)
            return false;

        const int W = Entry.Image.Width;
        const int H = Entry.Image.Height;
        const int Dim = W > H ? W : H;

        if (Filter.MinWidth && W < *Filter.MinWidth)   return false;
        if (Filter.MaxWidth && W > *Filter.MaxWidth)   return false;
        if (Filter.MinHeight && H < *Filter.MinHeight) return false;
        if (Filter.MaxHeight && H > *Filter.MaxHeight) return false;
        if (Filter.MinDim && Dim < *Filter.MinDim)     return false;
        if (Filter.MaxDim && Dim > *Filter.MaxDim)     return false;

        if (Filter.HasAlpha && Entry.Image.HasAlpha != *Filter.HasAlpha) return false;
        if (Filter.Cubemap && Entry.Image.Cubemap != *Filter.Cubemap)    return false;

        if (!Filter.Formats.empty())
        {
            if (std::find(Filter.Formats.begin(), Filter.Formats.end(), (int)Entry.Image.Format)
                == Filter.Formats.end())
                return false;
        }
    }

    const bool WantsModelMeta =
        Filter.MinBones || Filter.MaxBones || Filter.MinMeshes || Filter.MaxMeshes;

    if (WantsModelMeta)
    {
        if (Entry.Kind != AssetKind::Model)
            return false;

        const int B = (int)Entry.Model.Bones;
        const int M = (int)Entry.Model.Meshes;
        if (Filter.MinBones && B < *Filter.MinBones)   return false;
        if (Filter.MaxBones && B > *Filter.MaxBones)   return false;
        if (Filter.MinMeshes && M < *Filter.MinMeshes) return false;
        if (Filter.MaxMeshes && M > *Filter.MaxMeshes) return false;
    }

    // Free-text "name" query with Everything-style AND + '!' negation (the CLI-friendly form).
    if (!Filter.Name.empty() && !MatchNameQuery(Filter.Name, Entry.Name))
        return false;

    // Structured equivalents (kept for saved presets built programmatically).
    for (const auto& Term : Filter.NameContains)
        if (!Contains(Entry.Name, Term))
            return false;

    for (const auto& Term : Filter.NameExcludes)
        if (Contains(Entry.Name, Term))
            return false;

    if (!Filter.SourceContains.empty() && !Contains(Entry.Source, Filter.SourceContains))
        return false;

    return true;
}

bool LoadFilter(AssetFilter& Filter, const std::string& Path)
{
    std::ifstream In(Path, std::ios::binary);
    if (!In) return false;

    nlohmann::json J;
    try { In >> J; }
    catch (...) { return false; }
    if (!J.is_object()) return false;

    Filter = AssetFilter{};
    Filter.Name = J.value("name", std::string());
    if (J.contains("kind"))       Filter.Kind = AssetIndex::KindFromName(J["kind"].get<std::string>());
    if (J.contains("minWidth"))   Filter.MinWidth = J["minWidth"].get<int>();
    if (J.contains("maxWidth"))   Filter.MaxWidth = J["maxWidth"].get<int>();
    if (J.contains("minHeight"))  Filter.MinHeight = J["minHeight"].get<int>();
    if (J.contains("maxHeight"))  Filter.MaxHeight = J["maxHeight"].get<int>();
    if (J.contains("minDim"))     Filter.MinDim = J["minDim"].get<int>();
    if (J.contains("maxDim"))     Filter.MaxDim = J["maxDim"].get<int>();
    if (J.contains("hasAlpha"))   Filter.HasAlpha = J["hasAlpha"].get<bool>();
    if (J.contains("cubemap"))    Filter.Cubemap = J["cubemap"].get<bool>();
    if (J.contains("nameContains")) Filter.NameContains = J["nameContains"].get<std::vector<std::string>>();
    if (J.contains("nameExcludes")) Filter.NameExcludes = J["nameExcludes"].get<std::vector<std::string>>();
    if (J.contains("sourceContains")) Filter.SourceContains = J["sourceContains"].get<std::string>();
    if (J.contains("formats"))    Filter.Formats = J["formats"].get<std::vector<int>>();
    if (J.contains("minBones"))   Filter.MinBones = J["minBones"].get<int>();
    if (J.contains("maxBones"))   Filter.MaxBones = J["maxBones"].get<int>();
    if (J.contains("minMeshes"))  Filter.MinMeshes = J["minMeshes"].get<int>();
    if (J.contains("maxMeshes"))  Filter.MaxMeshes = J["maxMeshes"].get<int>();
    return true;
}
