#include "stdafx.h"

// Advanced Warfare's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameAdvancedWarfare.h"
#include "GameWorldWar2.h"

namespace
{
    class GameAdvancedWarfareLive final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "S1"; }
        const char* GetExportFolderName() const override { return "advanced_warfare"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameAdvancedWarfare::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameAdvancedWarfare::ReadXModel(Model);
        }
        std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound) const override
        {
            return GameWorldWar2::ReadXSound(Sound);
        }
        ViewmodelBoneConvention GetViewmodelBoneConvention() const override { return ViewmodelBoneConvention::Standard; }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits64; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::Standard; }
        VertexWeightsVariant GetVertexWeightsVariant() const override { return VertexWeightsVariant::B; }
        NormalUnpackVariant GetNormalUnpackVariant() const override { return NormalUnpackVariant::C; }
    };

    const GameAdvancedWarfareLive Game;
    const LiveGameRegistration Registered(Game);
}
