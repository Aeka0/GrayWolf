#include "stdafx.h"

#include "MaterialDescription.h"
#include "CoDAssets.h"

// WWII materials (MaterialDescription.h): the semantics the live reader names, colour,
// normal and the specular map (rgb the specular colour, alpha the gloss).
namespace
{
    class S2Materials final : public Materials::IAdapter
    {
    public:
        const char* Name() const override { return "ww2"; }
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
    const S2Materials Adapter;
    const Materials::AdapterRegistration Registered("S2", Adapter);
}
