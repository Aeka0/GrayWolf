#include "stdafx.h"

// Black Ops's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "CoDXModelTranslator.h"
#include "GameBlackOps.h"
#include "IWDSupport.h"

namespace
{
    class GameBlackOpsLive final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "T5"; }
        std::vector<PackageMark> Packages() const override { return { { PackageMark::Container::Iwd, 0, 0, "black ops" } }; }
        const char* GetExportFolderName() const override { return "black_ops_1"; }
        std::unique_ptr<XImageDDS> ReadFileImage(const CoDImage_t* Image) const override
        {
            return IWDSupport::ReadImageFile(Image);
        }
        // the disk path's readers (Games/T5/DiskGame.cpp, over a relocated zone)
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameBlackOps::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameBlackOps::ReadXModel(Model);
        }
        std::unique_ptr<WraithModel> TranslateHitboxModel(const std::unique_ptr<XModel_t>& GenericModel) const override
        {
            return CoDXModelTranslator::TranslateXModelHitbox(GenericModel);
        }
        ViewmodelBoneConvention GetViewmodelBoneConvention() const override { return ViewmodelBoneConvention::Standard; }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits32; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::Standard; }
        VertexWeightsVariant GetVertexWeightsVariant() const override { return VertexWeightsVariant::A; }
        NormalUnpackVariant GetNormalUnpackVariant() const override { return NormalUnpackVariant::A; }
    };

    const GameBlackOpsLive Game;
    const LiveGameRegistration Registered(Game);
}
