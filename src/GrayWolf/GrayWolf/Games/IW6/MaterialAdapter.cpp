#include "stdafx.h"

#include "MaterialDescription.h"
#include "CoDAssets.h"

// Ghosts materials (MaterialDescription.h): colour (BC3; its alpha is often not a mask,
// so the cutout is only automatic when that alpha is two-valued), normal (BC5 signed, x,y), and
// the specular map, rgb the specular colour and alpha the gloss. EB529B4D is a detail map.
namespace
{
    class IW6Materials final : public Materials::IAdapter
    {
    public:
        const char* Name() const override { return "ghosts"; }
        Materials::Description Describe(const Materials::Source& M) const override
        {
            using Materials::Role;
            return Materials::DescribeBySemantic(M, {
                { 0xA0AB1041, Role::Albedo },
                { 0x59D30D0F, Role::Normal },
                { 0x34ECCCB3, Role::Specular },
                { 0x34ECCCB3, Role::Gloss, 3 },
            });
        }
    };
    const IW6Materials Adapter;
    const Materials::AdapterRegistration Registered("IW6", Adapter);
}
