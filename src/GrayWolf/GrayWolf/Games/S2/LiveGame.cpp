#include "stdafx.h"

// WWII's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameWorldWar2.h"

namespace
{
    class GameWorldWar2Live final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "S2"; }
        const char* GetExportFolderName() const override { return "world_war_2"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameWorldWar2::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameWorldWar2::ReadXModel(Model);
        }
        std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound) const override
        {
            return GameWorldWar2::ReadXSound(Sound);
        }
        void LoadStreamedXModel(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel) const override
        {
            GameWorldWar2::LoadXModel(Model, ModelLOD, ResultModel);
        }
        ViewmodelBoneConvention GetViewmodelBoneConvention() const override { return ViewmodelBoneConvention::Standard; }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits64; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::Standard; }
    };

    const GameWorldWar2Live Game;
    const LiveGameRegistration Registered(Game);
}
