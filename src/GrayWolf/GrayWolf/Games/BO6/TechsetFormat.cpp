#include "stdafx.h"

// Black Ops 6's techsets (Techsets.h): its layouts, from the walker's tables (zonekit/profiles/bo6)
// and the loaders of cod24-cod.exe (RVAs below), and the arguments its shaders' reflection names.

#include "TechsetFormat.h"

#include <format>

namespace
{
    class Bo6 final : public Techsets::Format
    {
    public:
        Bo6()
        {
            // techset (0x60, 0x2848bc0): hash 0, seven u64 masks 0x20 (popcnt at 0x2848cc0), the
            // techniques 0x58 in mask order
            L.TechsetMasks = 0x20, L.TechsetMaskWords = 7, L.TechsetTechniques = 0x58;
            L.TechsetRaw = { { 0x8, 0x18 } };
            // technique (0xd0, 0x28485a0): shader records 0x48 vs, 0x50 ps, 0x58 hs, 0x60 ds (each
            // record's u64 at 8 says its stage, the DXIL program kind agrees), 0x68 never filled; the
            // arguments 0xa0 (u8 0x75; the bytes 0x70, 0x71, 0x72, 0x74 count the types 0, 1, 2, 11)
            L.TechniqueStages = { { 0x48, "vs" }, { 0x50, "ps" }, { 0x58, "hs" }, { 0x60, "ds" }, { 0x68, "gs" } };
            L.TechniqueArgs = 0xa0, L.TechniqueArgCount = 0x75;
            L.TechniqueRaw = { { 0x18, 0x30 }, { 0x70, 0x10 }, { 0x88, 0x18 }, { 0xa8, 0x28 } };
            // shader record (0x90): hash 0, its stage 8, the DXBC size 0x20, the bytecode record 0x28 (0x286d9e0)
            L.ShaderDxbcSize = 0x20, L.ShaderCode = 0x28;
            L.ShaderRaw = { { 0x8, 0x18 }, { 0x38, 0x58 } };
            // material (0x80, 0x2846c90): techset 0x28 (a decal volume's six images when the u64 at
            // 0x10 is 0x800000); image slots 0x30 (u8 0x1c), images 0x38 (u8 0x21), constant slots
            // 0x40 (u8 0x1d), constant rows 0x48 (u8 0x22); its block's size u32 0x78
            L.MaterialTechset = 0x28, L.MaterialImageSlots = 0x30, L.MaterialImageSlotCount = 0x1c, L.MaterialImages = 0x38, L.MaterialImageCount = 0x21;
            L.MaterialConstSlots = 0x40, L.MaterialConstSlotCount = 0x1d, L.MaterialConsts = 0x48, L.MaterialConstCount = 0x22;
            L.MaterialBlockSize = 0x78, L.MaterialKind = 0x10, L.MaterialNoTechset = 0x800000;
            L.MaterialRaw = { { 0x08, 0x14 }, { 0x50, 0x30 } };
            // the material buffer t38, space 1 (the lit shaders'); a block's header 40 bytes (the size
            // at 0x78 is 40 + 16 * constants + 4 * images on all 1 732 materials checked; every
            // direct texture index the eclipse map's lit shaders read falls in a block)
            L.MaterialBuffer = 38, L.BlockHeader = 40;
        }
        const Techsets::Layout& Layouts() const override { return L; }

        // Types 4, 5, 6: a code resource at t<dest>, space 2, 0, 1 (checked against the shaders'
        // reflection on every occurrence of code_post_gfx); t38, space 1 the material buffer. The
        // types 0, 1, 2, 3, 9 and 11 are not named yet.
        Techsets::Binding Bind(const Techsets::Argument& A) const override
        {
            switch (A.Type)
            {
            case 4: return { "code_texture", std::format("t{}", A.Dest), 2 };
            case 5: return { "code_buffer", std::format("t{}", A.Dest), 0 };
            case 6: return { A.Dest == L.MaterialBuffer ? "material_buffer" : "code_buffer", std::format("t{}", A.Dest), 1 };
            default: return { std::format("type{}", A.Type), "", -1 };
            }
        }

        // u16 index 8 (0..140), u16 flags 0xc, u16 variant 0xe (slot = variant * 141 + index); the
        // arrays at 0x10 (16 bytes) and 0x80 (5 bytes), u8 count 0x7d
        void TechniqueFields(const Techsets::ZoneView& Z, int64_t Pos, nlohmann::ordered_json& Extra, nlohmann::ordered_json& Raw) const override
        {
            const auto& B = *Z.Image;
            const auto U16 = [&](int64_t At) { return (uint16_t)(B[(size_t)At] | B[(size_t)At + 1] << 8); };
            Extra["flags"] = std::format("{:04x}", U16(Pos + 0xc));
            Extra["index"] = U16(Pos + 8);
            Extra["variant"] = U16(Pos + 0xe);
            const uint32_t Count = B[(size_t)(Pos + 0x7d)];
            if (const int64_t P = Z.Ptr(Pos + 0x10); P >= 0) Raw["0x10[]"] = Techsets::Hex(B, P, (int64_t)Count * 16);
            if (const int64_t P = Z.Ptr(Pos + 0x80); P >= 0) Raw["0x80[]"] = Techsets::Hex(B, P, (int64_t)Count * 5);
        }

    private:
        Techsets::Layout L;
    };
}

std::shared_ptr<const Techsets::Format> BO6::TechsetFormat()
{
    static const auto F = std::make_shared<Bo6>();
    return F;
}
