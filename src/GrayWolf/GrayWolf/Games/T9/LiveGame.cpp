#include "stdafx.h"

// Black Ops Cold War's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameBlackOpsCW.h"
#include "XPAKSupport.h"

namespace
{
    class GameBlackOpsCWLive final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "T9"; }
        const char* GetExportFolderName() const override { return "black_ops_cw"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameBlackOpsCW::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameBlackOpsCW::ReadXModel(Model);
        }
        std::unique_ptr<XImageDDS> ReadFileImage(const CoDImage_t* Image) const override
        {
            return XPAKSupport::ReadImageFile(Image);
        }
        std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound) const override
        {
            return GameBlackOpsCW::ReadXSound(Sound);
        }
        void LoadStreamedXModel(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel) const override
        {
            GameBlackOpsCW::LoadXModel(ModelLOD, ResultModel);
        }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits64; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::CW; }
        BlendShapeFormat GetBlendShapeFormat() const override { return BlendShapeFormat::BO4; }
    };

    const GameBlackOpsCWLive Game;
    const LiveGameRegistration Registered(Game);
}
