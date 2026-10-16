#include "stdafx.h"

// Black Ops II's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameBlackOps2.h"

namespace
{
    class GameBlackOps2Live final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "T6"; }
        std::vector<PackageMark> Packages() const override { return { { PackageMark::Container::Ipak }, { PackageMark::Container::Sab, 0xE } }; }
        const char* GetExportFolderName() const override { return "black_ops_2"; }
        // the disk path's readers (Games/T6/DiskGame.cpp, over a relocated zone)
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameBlackOps2::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameBlackOps2::ReadXModel(Model);
        }
        ViewmodelBoneConvention GetViewmodelBoneConvention() const override { return ViewmodelBoneConvention::Standard; }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits32; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::Standard; }
        VertexWeightsVariant GetVertexWeightsVariant() const override { return VertexWeightsVariant::A; }
        NormalUnpackVariant GetNormalUnpackVariant() const override { return NormalUnpackVariant::B; }
    };

    const GameBlackOps2Live Game;
    const LiveGameRegistration Registered(Game);
}
