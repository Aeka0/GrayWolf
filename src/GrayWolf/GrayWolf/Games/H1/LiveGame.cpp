#include "stdafx.h"

// Modern Warfare Remastered's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameModernWarfareRM.h"
#include "GameWorldWar2.h"

namespace
{
    class GameModernWarfareRMLive final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "H1"; }
        const char* GetExportFolderName() const override { return "modern_warfare_rm"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameModernWarfareRM::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameModernWarfareRM::ReadXModel(Model);
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

    const GameModernWarfareRMLive Game;
    const LiveGameRegistration Registered(Game);
}
