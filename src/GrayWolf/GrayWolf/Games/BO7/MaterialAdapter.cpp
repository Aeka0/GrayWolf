#include "stdafx.h"

#include "MaterialDescription.h"
#include "Games/IW/IwTerrain.h"

// Black Ops 7 materials: the default adapter's description (its name kept: the sidecars stay the
// same), the terrain's material layers added (Games/IW/IwTerrain)
namespace
{
    class BlackOps7Materials final : public Materials::IAdapter
    {
    public:
        const char* Name() const override { return Materials::DefaultAdapter().Name(); }
        Materials::Description Describe(const Materials::Source& M) const override
        {
            Materials::Description D = Materials::DefaultAdapter().Describe(M);
            IwTerrain::DescribeLayers(M, D);
            return D;
        }
    };
    const BlackOps7Materials Adapter;
    const Materials::AdapterRegistration Registered("BO7", Adapter);
}
