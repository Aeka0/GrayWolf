#include "stdafx.h"

#include "MaterialDescription.h"
#include "CoDAssets.h"

#include <string>

// Black Ops 4 materials: the same semantics as Black Ops 3 (MaterialDescription.h).
// Colour BC7 sRGB, normal BC7 with x,y in r,g (b holds something else: z is rebuilt), gloss and
// occlusion BC4, specular colour BC1. 4E07583B/AB07D475/01782DE2 are the character shader's.
// Hair: the tint gradient (1C3FEDDD), and a colour mixed in by a mask, as the technique set's
// shader reads them: the bo4 profile's shading.txt and the constant blob.
namespace
{
    constexpr uint32_t TintGradient = 0x1C3FEDDD;

    int ChannelOf(const std::string& C) { const size_t I = std::string("rgba").find(C.empty() ? 'r' : C[0]); return I == std::string::npos ? 0 : (int)I; }

    void DescribeTint(const Materials::Source& M, Materials::Description& D)
    {
        if (M.Techset.empty() || M.Constants.empty()) return;
        const Materials::Recipe* R = Materials::ShadingRecipe("bo4", "0x" + M.Techset);
        if (R == nullptr) return;
        const std::vector<uint8_t>& Blob = M.Constants[0];
        const auto Field = [&](const char* K) { const auto It = R->find(K); return It == R->end() ? std::string() : It->second; };
        // a colour mixed in by a mask (color), then the gradient (tint)
        const std::string MaskSemantic = Field("tint_mask");
        if (const Materials::Source::Image* Mask = MaskSemantic.empty() ? nullptr : M.BySemantic((uint32_t)std::stoul(MaskSemantic, nullptr, 16)))
        {
            D.Color.Mask = { Mask->Path, ChannelOf(Field("tint_mask_channel")), false };
            for (int c = 0; c < 3; c++) D.Color.Color[c] = Materials::RecipeFloat(*R, "tint_color", Blob, c, 1.0f);
        }
        const Materials::Source::Image* Gradient = M.BySemantic(TintGradient);
        if (Gradient == nullptr || Field("scale").empty()) return;
        auto& T = D.Tint;
        T.Image.Path = Gradient->Path;
        T.Rgb = Field("channel") == "rgb";
        T.Image.Channel = T.Rgb ? 0 : ChannelOf(Field("channel"));
        T.U = 0.5f;
        const std::string OffsetMap = Field("offset_map");
        if (const Materials::Source::Image* Offset = OffsetMap.empty() ? nullptr : M.BySemantic((uint32_t)std::stoul(OffsetMap, nullptr, 16)))
        {
            T.Offset = { Offset->Path, ChannelOf(Field("offset_channel")), false };
            T.OffsetScale = Materials::RecipeFloat(*R, "scale", Blob, 0, 0.0f);
        }
        if (!T.Rgb)
            for (int c = 0; c < 3; c++) T.Color[c] = Materials::RecipeFloat(*R, "color", Blob, c, 1.0f);
    }

    class T8Materials final : public Materials::IAdapter
    {
    public:
        const char* Name() const override { return "bo4"; }
        Materials::Description Describe(const Materials::Source& M) const override
        {
            using Materials::Role;
            Materials::Description D = Materials::DescribeBySemantic(M, {
                { 0xA0AB1041, Role::Albedo },   { 0x4E07583B, Role::Albedo },
                { 0x59D30D0F, Role::Normal },   { 0xAB07D475, Role::Normal },
                { 0x6D0A6C98, Role::Gloss },    { 0x01782DE2, Role::Gloss },
                { 0xEC443804, Role::Specular }, { 0x07176BF2, Role::Occlusion },
                { 0x34614347, Role::Emissive },
            });
            DescribeTint(M, D);
            return D;
        }
    };
    const T8Materials Adapter;
    const Materials::AdapterRegistration Registered("T8", Adapter);
}
