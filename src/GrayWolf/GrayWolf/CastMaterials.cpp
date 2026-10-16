#include "stdafx.h"

#include "CastMaterials.h"
#include "CastExport.h"

#include <filesystem>

namespace Materials
{
namespace
{
    // "..\..\images\x.dds" as the model reader spells it -> "../../images/x.dds"
    std::string Portable(const std::string& Path)
    {
        return std::filesystem::path(Path).lexically_normal().generic_string();
    }
}

CastMaterialSlots CastSlots(const Description& D)
{
    CastMaterialSlots S;
    S.Name = D.Name;
    const auto Add = [&S](const char* Slot, const Map& M) {
        if (!M.Empty()) S.Files.emplace_back(Slot, Portable(M.Path));
    };
    Add("albedo", D.Albedo);
    Add("normal", D.Normal);
    // a gloss map read as roughness stays a gloss map: the importers invert it themselves
    Add(D.Roughness.Invert ? "gloss" : "roughness", D.Roughness);
    Add("metal", D.Metalness);
    Add("specular", D.Specular);
    Add("ao", D.Occlusion);
    Add("emissive", D.Emissive);

    int Extra = 0;
    const auto AddExtra = [&](const Map& M) {
        if (!M.Empty()) S.Files.emplace_back("extra" + std::to_string(Extra++), Portable(M.Path));
    };
    AddExtra(D.Color.Mask);
    AddExtra(D.Tint.Image);
    AddExtra(D.Tint.Offset);
    for (const Layer& L : D.Layers)
    {
        AddExtra(L.Albedo);
        AddExtra(L.Normal);
        AddExtra(L.Roughness);
        AddExtra(L.Mask);
    }
    AddExtra(D.Transmission.Image);   // Cast has no transmission or opacity slot
    AddExtra(D.AlphaMap.Image);
    return S;
}

std::vector<CastMaterialSlots> CastSlots(const std::vector<Description>& Materials)
{
    std::vector<CastMaterialSlots> Out;
    Out.reserve(Materials.size());
    for (const Description& D : Materials) Out.push_back(CastSlots(D));
    return Out;
}
}
