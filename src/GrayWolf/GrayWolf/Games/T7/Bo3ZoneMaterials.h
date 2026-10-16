#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Material (type 6) texture-slot capture. See Bo3ZoneMaterials.cpp for what turns this into an
// export file. It mirrors the material's texture sub-table (MaterialTextureDef, 32 bytes/entry: +0x00 ImagePtr, +0x08 semantic).
struct Bo3MaterialTextureSlotCapture
{
    // MaterialTextureDef's semantic hash - the same value GameBlackOps3::ReadXMaterial's
    // switch maps to Diffuse/Normal/Specular via 0xA0AB1041/0x59D30D0F/0xEC443804
    // (see that function in GameBlackOps3.cpp). Kept raw here; usage is classified at export.
    uint32_t SemanticHash = 0;
    // The texture's own name, resolved from its embedded GfxImage when this slot's entry pointer
    // was inline (FollowsInline - real content is written to the zone right here). Empty when the
    // slot instead held an already-resolved reference to a GfxImage loaded elsewhere in the zone
    // (Inline == false): this material's own bytes carry no name for that case.
    std::string Name;
    bool Inline = false;
};

// Populated only for TypeMaterial assets walked with a resolved Name: a nameless
// embedded/reference Material has nothing an exporter could file under.
struct Bo3MaterialCapture
{
    bool Valid = false;
    // The material's own embedded TechniqueSet name, when inline (FollowsInline/FollowsInline2) -
    // empty for a resolved reference to an already-loaded TechniqueSet (same "no name recoverable"
    // situation as a non-inline texture slot above).
    std::string TechniqueSetName;
    std::vector<Bo3MaterialTextureSlotCapture> Textures;
};

// One material captured from the zonekit-walked positions (Games/T7/DiskGame.cpp's ExportData),
// written as JSON. True when written.
bool Bo3WriteMaterialJson(const std::string& Name, const Bo3MaterialCapture& Mat,
                          const std::string& ImagesDir, const std::string& OutDir,
                          size_t& TexturesTotal, size_t& TexturesResolved, size_t& TexturesWithPng);
