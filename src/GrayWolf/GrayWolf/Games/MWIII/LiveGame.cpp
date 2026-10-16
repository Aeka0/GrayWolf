#include "stdafx.h"

// Modern Warfare III (2023)'s live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameModernWarfare6.h"

namespace
{
    class GameModernWarfare6Live final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "MWIII"; }
        bool ReadsOwnBones() const override { return true; }
        const char* GetExportFolderName() const override { return "modern_warfare_6"; }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameModernWarfare6::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameModernWarfare6::ReadXModel(Model);
        }
        std::unique_ptr<XImageDDS> ReadXImage(const CoDImage_t* Image) const override
        {
            return GameModernWarfare6::ReadXImage(Image);
        }
        std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound) const override
        {
            return GameModernWarfare6::ReadXSound(Sound);
        }
        void LoadStreamedXModel(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD, const std::unique_ptr<WraithModel>& ResultModel) const override
        {
            GameModernWarfare6::LoadXModel(Model, ModelLOD, ResultModel);
        }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits64; }
    };

    const GameModernWarfare6Live Game;
    const LiveGameRegistration Registered(Game);
}
