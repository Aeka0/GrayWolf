#include "stdafx.h"

// Infinite Warfare's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameInfiniteWarfare.h"
#include "SABSupport.h"

namespace
{
    class GameInfiniteWarfareLive final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "IW7"; }
        std::vector<PackageMark> Packages() const override { return { { PackageMark::Container::Sab, 0x4 } }; }
        const char* GetExportFolderName() const override { return "infinite_warfare"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameInfiniteWarfare::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameInfiniteWarfare::ReadXModel(Model);
        }
        std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound) const override
        {
            return SABSupport::LoadSound(Sound);
        }
        ViewmodelBoneConvention GetViewmodelBoneConvention() const override { return ViewmodelBoneConvention::InfiniteWarfare; }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits64; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::Standard; }
        VertexWeightsVariant GetVertexWeightsVariant() const override { return VertexWeightsVariant::C; }
        NormalUnpackVariant GetNormalUnpackVariant() const override { return NormalUnpackVariant::C; }
    };

    const GameInfiniteWarfareLive Game;
    const LiveGameRegistration Registered(Game);
}
