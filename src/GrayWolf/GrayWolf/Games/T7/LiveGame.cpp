#include "stdafx.h"

// Black Ops III's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "CoDXModelTranslator.h"
#include "GameBlackOps3.h"
#include "SABSupport.h"
#include "XPAKSupport.h"

namespace
{
    class GameBlackOps3Live final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "T7"; }
        std::vector<PackageMark> Packages() const override { return { { PackageMark::Container::Sab, 0xF }, { PackageMark::Container::Xpak, 0xA } }; }
        const char* GetExportFolderName() const override { return "black_ops_3"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameBlackOps3::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameBlackOps3::ReadXModel(Model);
        }
        std::unique_ptr<XImageDDS> ReadFileImage(const CoDImage_t* Image) const override
        {
            return XPAKSupport::ReadImageFile(Image);
        }
        std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound) const override
        {
            return SABSupport::LoadSound(Sound);
        }
        std::unique_ptr<WraithModel> TranslateHitboxModel(const std::unique_ptr<XModel_t>& GenericModel) const override
        {
            return CoDXModelTranslator::TranslateXModelHitbox(GenericModel);
        }
        void LoadStreamedXModel(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel) const override
        {
            GameBlackOps3::LoadXModel(ModelLOD, ResultModel);
        }
        ViewmodelBoneConvention GetViewmodelBoneConvention() const override { return ViewmodelBoneConvention::BO3BO4; }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits64; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::BO3; }
    };

    const GameBlackOps3Live Game;
    const LiveGameRegistration Registered(Game);
}
