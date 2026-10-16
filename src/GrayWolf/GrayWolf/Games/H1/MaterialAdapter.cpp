#include "stdafx.h"

#include "MaterialDescription.h"
#include "CoDAssets.h"

// Modern Warfare Remastered materials (MaterialDescription.h): the semantics the live
// reader names (GameModernWarfareRM::ReadXMaterial): colour, normal (BC5, x,y) and specular.
namespace
{
    class H1Materials final : public Materials::IAdapter
    {
    public:
        const char* Name() const override { return "mwr"; }
        Materials::Description Describe(const Materials::Source& M) const override
        {
            using Materials::Role;
            return Materials::DescribeBySemantic(M, {
                { 0xA0AB1041, Role::Albedo },
                { 0x59D30D0F, Role::Normal },
                { 0x34ECCCB3, Role::Specular },
            });
        }
    };
    const H1Materials Adapter;
    const Materials::AdapterRegistration Registered("H1", Adapter);
}
