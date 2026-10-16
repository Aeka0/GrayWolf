#include "stdafx.h"

// Call of Duty Online's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "Games/CODOL/CodolReaders.h"

namespace
{
    // Call of Duty Online's zones, served from disk (Games/CODOL/CodolReaders.h): MW2's 32-bit shapes
    class GameOnlineLive final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "CODOL"; }
        const char* GetExportFolderName() const override { return "call_of_duty_online"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return Codol::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return Codol::ReadXModel(Model);
        }
        ViewmodelBoneConvention GetViewmodelBoneConvention() const override { return ViewmodelBoneConvention::Standard; }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits32; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::Standard; }
        VertexWeightsVariant GetVertexWeightsVariant() const override { return VertexWeightsVariant::A; }
        NormalUnpackVariant GetNormalUnpackVariant() const override { return NormalUnpackVariant::A; }
    };

    const GameOnlineLive Game;
    const LiveGameRegistration Registered(Game);
}
