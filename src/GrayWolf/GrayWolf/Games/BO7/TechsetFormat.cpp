#include "stdafx.h"

// Black Ops 7's techsets (Techsets.h): its layouts, from the walker's tables (zonekit/profiles/bo7:
// techset f3870730, technique f38701c0, shader f38701c0_e1 / f389e6e0, material f386e2d0; the
// loaders of cod.exe 0x6aadce0e), the arguments as the shaders' bindings prove them, and its
// layered materials' blocks.

#include "TechsetFormat.h"

#include <algorithm>
#include <format>

namespace
{
    class Bo7 final : public Techsets::Format
    {
    public:
        Bo7()
        {
            // techset (0x50): hash 0, five u64 masks 0x18 (a technique per set bit: its slot, 0 to
            // 319), the techniques 0x40 in mask order
            L.TechsetMasks = 0x18, L.TechsetMaskWords = 5, L.TechsetTechniques = 0x40;
            L.TechsetRaw = { { 0x8, 0x10 }, { 0x48, 8 } };
            // technique (0xc8): shader records 0x38 vs, 0x40 ps, 0x48 hs and 0x50 ds (the 72
            // tessellated techniques), 0x58 never seen set; the arguments 0x80 (u8 0xbe: u8 0xbb
            // constants first, then u8 0xbc resources, then u8 0xbd others)
            L.TechniqueStages = { { 0x38, "vs" }, { 0x40, "ps" }, { 0x48, "hs" }, { 0x50, "ds" }, { 0x58, "s58" } };
            L.TechniqueArgs = 0x80, L.TechniqueArgCount = 0xbe;
            L.TechniqueRaw = { { 0x88, 0x40 } };
            // shader record (0x70): hash 0, the bytecode record 0x18, the DXBC size u32 0x2c
            L.ShaderDxbcSize = 0x2c, L.ShaderCode = 0x18;
            L.ShaderRaw = { { 0x30, 0x40 } };
            // material (0x78): techset 0x20 (something else when the u64 at 0x18 is 0x1000000: the
            // table's other branch, f3853680); image slots 0x28 (u8 0x68), images 0x30 (u8 0x6d),
            // constant slots 0x38 (u8 0x69), constant rows 0x40 (u8 0x6e)
            L.MaterialTechset = 0x20, L.MaterialImageSlots = 0x28, L.MaterialImageSlotCount = 0x68, L.MaterialImages = 0x30, L.MaterialImageCount = 0x6d;
            L.MaterialConstSlots = 0x38, L.MaterialConstSlotCount = 0x69, L.MaterialConsts = 0x40, L.MaterialConstCount = 0x6e;
            L.MaterialKind = 0x18, L.MaterialNoTechset = 0x1000000;
            L.MaterialRaw = { { 0x48, 0x20 } };
            // the material buffer t42, space 1 (the code buffer 42 the lit techniques bind); a block's
            // header 40 bytes the code fills (+36: the layer count)
            L.MaterialBuffer = 42, L.BlockHeader = 40;
        }
        const Techsets::Layout& Layouts() const override { return L; }

        // Over every technique of techsets_global, _shared_mp, _stream_mp, ingame_mp, the liminal
        // map's and code_post_gfx (the stage's resource at dest): types 0, 1, 2 a code constant
        // (source: its id) into row dest of the cbuffer perPrim, perObject or stable; 3 a code
        // texture t<dest>; 4 one at t<dest>, space 2; 5 a code buffer t<dest>; 6 one at t<dest>,
        // space 1 (t42 the material buffer); 7 a read-write texture u<dest>; 8 a read-write buffer
        // u<dest>; 9 a code cbuffer b<dest> (b3: scene); 11 a buffer t<dest>
        Techsets::Binding Bind(const Techsets::Argument& A) const override
        {
            switch (A.Type)
            {
            case 0: return { "code_constant", std::format("perPrim[{}]", A.Dest), 0 };
            case 1: return { "code_constant", std::format("perObject[{}]", A.Dest), 0 };
            case 2: return { "code_constant", std::format("stable[{}]", A.Dest), 0 };
            case 3: return { "code_texture", std::format("t{}", A.Dest), 0 };
            case 4: return { "code_texture", std::format("t{}", A.Dest), 2 };
            case 5: return { "code_buffer", std::format("t{}", A.Dest), 0 };
            case 6: return { A.Dest == L.MaterialBuffer ? "material_buffer" : "code_buffer", std::format("t{}", A.Dest), 1 };
            case 7: return { "code_rwtexture", std::format("u{}", A.Dest), 0 };
            case 8: return { "code_rwbuffer", std::format("u{}", A.Dest), 0 };
            case 9: return { "code_cbuffer", std::format("b{}", A.Dest), 0 };
            case 11: return { "buffer", std::format("t{}", A.Dest), 0 };
            default: return { std::format("type{}", A.Type), "", -1 };
            }
        }

        // A layered material (semantics from 0x47 and constant ids from 0x5a by layer, u8 0x6a images
        // and u8 0x6b constants each) keeps its global entries first, then each layer's constants and
        // images. Proved on the shaders' reads of the material buffer: mp_sat_sm_liminal 17 087 of
        // 17 087 texture indices on an image slot of this layout, mp_sat_sm_hospital 21 803 / 21 803,
        // code_post_gfx 4 550 / 4 550, zm_sat_ztron4 348 067 / 348 082 (the 15 others: eye shaders
        // reading an index at +0x68, among the constants).
        Techsets::Block BlockOf(const Techsets::ZoneView& Z, const Techsets::MaterialSlots& M) const override
        {
            const auto& B = *Z.Image;
            const uint32_t NS = (uint32_t)M.ImageSemantics.size(), NI = (uint32_t)M.ConstantSemantics.size();
            const uint32_t PerImages = B[(size_t)(M.Pos + 0x6a)], PerConsts = B[(size_t)(M.Pos + 0x6b)];
            const uint32_t GS = (uint32_t)std::count_if(M.ImageSemantics.begin(), M.ImageSemantics.end(), [](uint8_t V) { return V < 0x47; });
            const uint32_t GI = (uint32_t)std::count_if(M.ConstantSemantics.begin(), M.ConstantSemantics.end(), [](uint8_t V) { return V < 0x5a; });
            if (PerImages == 0 || PerConsts == 0 || NS <= GS || (NS - GS) % PerImages != 0 || (NI - GI) % PerConsts != 0 ||
                (NS - GS) / PerImages != (NI - GI) / PerConsts)
                return Format::BlockOf(Z, M);
            Techsets::Block Bk;
            Bk.Layers = (NS - GS) / PerImages;
            const uint32_t H = L.BlockHeader, Stride = 16 * PerConsts + 4 * PerImages, Layered = H + 16 * GI + 4 * GS;
            for (uint32_t J = 0; J < NI; J++) Bk.Constants.push_back(J < GI ? H + 16 * J : Layered + (J - GI) / PerConsts * Stride + 16 * ((J - GI) % PerConsts));
            for (uint32_t K = 0; K < NS; K++)
                Bk.Images.push_back(K < GS ? H + 16 * GI + 4 * K : Layered + (K - GS) / PerImages * Stride + 16 * PerConsts + 4 * ((K - GS) % PerImages));
            return Bk;
        }

        // the bytes 0x68..0x80 ("flags"); the 16-byte records 0x8 and the 8-byte records 0x60 (u8
        // count 0xc0); u8 0xa8 a base slot its variants share (119 and 220 keep 18: the slot is the
        // mask's bit); the arguments by group
        void TechniqueFields(const Techsets::ZoneView& Z, int64_t Pos, nlohmann::ordered_json& Extra, nlohmann::ordered_json& Raw) const override
        {
            const auto& B = *Z.Image;
            Extra["flags"] = Techsets::Hex(B, Pos + 0x68, 0x18);
            const uint32_t Count = B[(size_t)(Pos + 0xc0)];
            if (const int64_t P = Z.Ptr(Pos + 0x8); P >= 0) Raw["0x08[]"] = Techsets::Hex(B, P, (int64_t)Count * 16);
            if (const int64_t P = Z.Ptr(Pos + 0x60); P >= 0) Raw["0x60[]"] = Techsets::Hex(B, P, (int64_t)Count * 8);
            Raw["0xa8"] = std::format("{:x}", B[(size_t)(Pos + 0xa8)]);
            Raw["arg_groups"] = std::format("{} constants, {} resources, {} last", B[(size_t)(Pos + 0xbb)], B[(size_t)(Pos + 0xbc)], B[(size_t)(Pos + 0xbd)]);
        }

    private:
        Techsets::Layout L;
    };
}

std::shared_ptr<const Techsets::Format> BO7::TechsetFormat()
{
    static const auto F = std::make_shared<Bo7>();
    return F;
}
