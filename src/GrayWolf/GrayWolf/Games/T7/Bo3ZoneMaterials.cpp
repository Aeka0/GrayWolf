#include "stdafx.h"

#include "Bo3ZoneMaterials.h"

#include "ExportNaming.h"
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

namespace
{
    namespace fs = std::filesystem;

    using ExportNaming::SafeFileName;

    // Same magic constants GameBlackOps3::ReadXMaterial's switch uses
    // (GameBlackOps3.cpp) - classification is export-time, the walker's own capture stays raw.
    const char* UsageName(uint32_t SemanticHash)
    {
        switch (SemanticHash)
        {
        case 0xA0AB1041u: return "diffuse";
        case 0x59D30D0Fu: return "normal";
        case 0xEC443804u: return "specular";
        default: return "unknown";
        }
    }
}

bool Bo3WriteMaterialJson(const std::string& Name, const Bo3MaterialCapture& Mat,
                          const std::string& ImagesDir, const std::string& OutDir,
                          size_t& TexturesTotal, size_t& TexturesResolved, size_t& TexturesWithPng)
{
    std::error_code Ec;
    const bool HaveImagesDir = !ImagesDir.empty() && fs::is_directory(ImagesDir, Ec);

    nlohmann::json J;
    J["material"] = Name;
    if (!Mat.TechniqueSetName.empty())
        J["techniqueSet"] = Mat.TechniqueSetName;

    nlohmann::json Textures = nlohmann::json::array();
    for (const auto& Tex : Mat.Textures)
    {
        TexturesTotal++;

        nlohmann::json T;
        T["semanticHash"] = Tex.SemanticHash;
        T["usage"] = UsageName(Tex.SemanticHash);
        T["inline"] = Tex.Inline;

        if (!Tex.Name.empty())
        {
            TexturesResolved++;
            T["name"] = Tex.Name;

            if (HaveImagesDir)
            {
                const auto PngPath = fs::path(ImagesDir) / (SafeFileName(Tex.Name) + ".png");
                const bool HasPng = fs::exists(PngPath, Ec);
                T["imageFile"] = HasPng ? PngPath.filename().string() : std::string();
                T["imagePresent"] = HasPng;
                if (HasPng) TexturesWithPng++;
            }
        }

        Textures.push_back(std::move(T));
    }
    J["textures"] = std::move(Textures);

    std::error_code MkEc;
    fs::create_directories(OutDir, MkEc);
    const std::string Path = (fs::path(OutDir) / (SafeFileName(Name) + ".json")).string();
    std::ofstream Out(Path, std::ios::binary | std::ios::trunc);
    if (Out) Out << J.dump(2);
    return (bool)Out;
}
