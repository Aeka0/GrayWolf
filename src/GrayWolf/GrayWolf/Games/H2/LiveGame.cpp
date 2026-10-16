#include "stdafx.h"

// Modern Warfare 2 Campaign Remastered's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameModernWarfare2RM.h"
#include "GameWorldWar2.h"

namespace
{
    class GameModernWarfare2RMLive final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "H2"; }
        const char* GetExportFolderName() const override { return "modern_warfare_2_rm"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameModernWarfare2RM::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameModernWarfare2RM::ReadXModel(Model);
        }
        std::unique_ptr<XImageDDS> ReadXImage(const CoDImage_t* Image) const override
        {
            return GameModernWarfare2RM::ReadXImage(Image);
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

    const GameModernWarfare2RMLive Game;
    const LiveGameRegistration Registered(Game);
}
