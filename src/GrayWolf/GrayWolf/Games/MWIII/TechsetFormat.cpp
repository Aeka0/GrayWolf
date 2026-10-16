#include "stdafx.h"

// Modern Warfare III's techsets (Techsets.h): its layouts, from the walker's tables
// (zonekit/profiles/mwiii) and the loaders of cod23-cod.exe (2026-09, RVAs below), and what its
// technique arguments bind, as the shaders' reflection shows it on every occurrence.

#include "TechsetFormat.h"

#include <format>

namespace
{
    class Mwiii final : public Techsets::Format
    {
    public:
        Mwiii()
        {
            // techset (0x30, 0x3af67a0): hash 0, two u64 masks 0x18 (popcnt at 0x3af6860), a
            // technique per set bit at 0x28
            L.TechsetMasks = 0x18, L.TechsetMaskWords = 2, L.TechsetTechniques = 0x28;
            L.TechsetRaw = { { 0x8, 8 }, { 0x10, 8 } };
            // technique (0xa8, 0x3af64a0): the shader records 0x48 vs, 0x50 hs, 0x58 ds, 0x60 ps (their
            // loaders 0x3af6f70, 0x3af5ae0, 0x3af5860, 0x3af5ba0 in the exe's order of
            // Material{Vertex,Hull,Domain,Pixel}Shader; the DXIL program kind of all 1 856 blobs
            // agrees); the arguments 0x98 (u8 0x6d: 0x6a of type 2, 0x6b of types 3..10, 0x6c 11 on)
            L.TechniqueStages = { { 0x48, "vs" }, { 0x50, "hs" }, { 0x58, "ds" }, { 0x60, "ps" } };
            L.TechniqueArgs = 0x98, L.TechniqueArgCount = 0x6d;
            L.TechniqueRaw = { { 0x10, 8 }, { 0x68, 8 }, { 0x70, 8 }, { 0x80, 0x18 }, { 0xa0, 8 } };
            // shader record (0x58): hash 0, its HLSL file name 8, the DXBC size u32 0x20, the
            // bytecode record 0x28 (0x3b107a0)
            L.ShaderName = 8, L.ShaderDxbcSize = 0x20, L.ShaderCode = 0x28;
            L.ShaderRaw = { { 0x10, 0x10 }, { 0x24, 4 }, { 0x38, 0x20 } };
            // material (0x78, 0x3af4f80): techset 0x20 (a decal volume's material when u32 0x10 is
            // 0x800000); image slots 0x28 (u8 0x18), images 0x30 (u8 0x1b), constant slots 0x38 (u8
            // 0x19), constant rows 0x40 (u8 0x1c); its block's size u32 0x6c
            L.MaterialTechset = 0x20, L.MaterialImageSlots = 0x28, L.MaterialImageSlotCount = 0x18, L.MaterialImages = 0x30, L.MaterialImageCount = 0x1b;
            L.MaterialConstSlots = 0x38, L.MaterialConstSlotCount = 0x19, L.MaterialConsts = 0x40, L.MaterialConstCount = 0x1c;
            L.MaterialBlockSize = 0x6c, L.MaterialKind = 0x10, L.MaterialKindMask = 0xffffffff, L.MaterialNoTechset = 0x800000;
            // the material buffer t39, space 1; a block's header 36 bytes (the block's size at 0x6c is
            // 36 + 16 * constants + 4 * images on every material of code_post_gfx and shipment; every
            // texture index the shaders read lands on an image slot)
            L.MaterialBuffer = 39, L.BlockHeader = 36;
        }
        const Techsets::Layout& Layouts() const override { return L; }

        // Type 2 a code constant <source> written at row <dest> of the per-draw cbuffer b2 (every
        // dest of 146 788 is inside the shader's b2); 4 the material's image of semantic <source>
        // at t<dest>, space 2; 6 its constant of semantic <source> at t<dest>, space 1; 3, 5, 11
        // a code resource t<dest>, space 0; 7 a code UAV u<dest>; 9 a sampler s<dest>
        Techsets::Binding Bind(const Techsets::Argument& A) const override
        {
            using S = Techsets::Binding::Source;
            switch (A.Type)
            {
            case 2: return { "code_constant", std::format("b2+0x{:x}", A.Dest * 16), 0 };
            case 3: return { "code_texture", std::format("t{}", A.Dest), 0 };
            case 4: return { "material_texture", std::format("t{}", A.Dest), 2, S::MaterialTexture };
            case 5: return { "code_buffer", std::format("t{}", A.Dest), 0 };
            case 6: return { "material_constant", std::format("t{}", A.Dest), 1, S::MaterialConstant };
            case 7: return { "code_uav", std::format("u{}", A.Dest), 0 };
            case 9: return { "sampler", std::format("s{}", A.Dest), 0, S::Sampler };
            case 11: return { "code_raw", std::format("t{}", A.Dest), 0 };
            default: return { std::format("type{}", A.Type), "", -1 };
            }
        }

        // the u64 at 8 (its slot byte first); the arrays the counts at 0xf and 0x75 size
        void TechniqueFields(const Techsets::ZoneView& Z, int64_t Pos, nlohmann::ordered_json& Extra, nlohmann::ordered_json& Raw) const override
        {
            const auto& B = *Z.Image;
            const auto Hex = [&](int64_t At, int64_t N) { return Techsets::Hex(B, At, N); };
            uint64_t Flags = 0;
            if (Pos + 16 <= (int64_t)B.size()) std::memcpy(&Flags, B.data() + Pos + 8, 8);
            Extra["flags"] = std::format("{:x}", Flags);
            const uint32_t Passes = B[(size_t)(Pos + 0x75)], States = B[(size_t)(Pos + 0xf)];
            if (const int64_t P = Z.Ptr(Pos + 0x18); P >= 0) Raw["0x18[]"] = Hex(P, (int64_t)Passes * States * 16);
            if (const int64_t P = Z.Ptr(Pos + 0x40); P >= 0) Raw["0x40[]"] = Hex(P, 10);
            if (const int64_t P = Z.Ptr(Pos + 0x78); P >= 0) Raw["0x78[]"] = Hex(P, (int64_t)Passes * 5);
        }

        bool TechniquesBindMaterials() const override { return true; }

    private:
        Techsets::Layout L;
    };
}

std::shared_ptr<const Techsets::Format> MWIII::TechsetFormat()
{
    static const auto F = std::make_shared<Mwiii>();
    return F;
}
