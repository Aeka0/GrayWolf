#pragma once

#include <string>
#include <vector>
#include <optional>

#include "AssetIndex.h"

// A saved, reusable query over the asset index. Every set predicate must hold (AND). Unset
// predicates are ignored. Image-specific predicates (dimensions, alpha, cubemap, format) only
// match image entries, so asking for e.g. alpha implicitly restricts to images.
//
// Example - "small 2D HUD icon with transparency": Kind=image, MaxDim=128, HasAlpha=true,
// Cubemap=false, optionally NameContains=["hud"] or SourceContains a map package.
struct AssetFilter
{
    std::string Name;                       // preset name (for save/load UI)

    std::optional<AssetKind> Kind;

    std::optional<int> MinWidth, MaxWidth;
    std::optional<int> MinHeight, MaxHeight;
    std::optional<int> MinDim, MaxDim;      // on max(width, height) - "at least/at most this big"

    std::optional<bool> HasAlpha;
    std::optional<bool> Cubemap;

    std::vector<std::string> NameContains;  // all must appear (case-insensitive)
    std::vector<std::string> NameExcludes;  // none may appear
    std::string SourceContains;             // package/source substring

    std::vector<int> Formats;               // allowed IWi format bytes (empty = any)

    // Model predicates (imply Kind == model).
    std::optional<int> MinBones, MaxBones;  // e.g. MaxBones=1 -> static props
    std::optional<int> MinMeshes, MaxMeshes;
};

// True if Entry satisfies every set predicate of Filter.
bool MatchesFilter(const AssetFilter& Filter, const AssetEntry& Entry);

bool LoadFilter(AssetFilter& Filter, const std::string& Path);
