#include "stdafx.h"

// Black Ops 4's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameBlackOps4.h"
#include "SABSupport.h"
#include "XPAKSupport.h"

namespace
{
    class GameBlackOps4Live final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "T8"; }
        std::vector<PackageMark> Packages() const override { return { { PackageMark::Container::Sab, 0x15 }, { PackageMark::Container::Xpak, 0xB } }; }
        const char* GetExportFolderName() const override { return "black_ops_4"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameBlackOps4::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameBlackOps4::ReadXModel(Model);
        }
        std::unique_ptr<XImageDDS> ReadFileImage(const CoDImage_t* Image) const override
        {
            return XPAKSupport::ReadImageFile(Image);
        }
        std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound) const override
        {
            return SABSupport::LoadSound(Sound);
        }
        void LoadStreamedXModel(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel) const override
        {
            GameBlackOps4::LoadXModel(ModelLOD, ResultModel);
        }
        ViewmodelBoneConvention GetViewmodelBoneConvention() const override { return ViewmodelBoneConvention::BO3BO4; }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits64; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::BO4; }
        BlendShapeFormat GetBlendShapeFormat() const override { return BlendShapeFormat::BO4; }
    };

    const GameBlackOps4Live Game;
    const LiveGameRegistration Registered(Game);
}
