#include "stdafx.h"

// Black Ops 6's readers, as the common code reaches them (ICoDGame.h): run over its zones served
// from disk (Games/BO6/DiskGame.cpp), there is no running-game path.

#include "ICoDGame.h"
#include "GameBlackOps6.h"

namespace
{
    using LayoutScope = GameBlackOps6::LayoutScope;

    // "BO6" the multiplayer and zombies (cod24), "BO6SP" the campaign (sp24): one set of readers,
    // the campaign's few moved fields by its layout
    class GameBlackOps6Live final : public ICoDGame
    {
    public:
        GameBlackOps6Live(const char* Name, const GameBlackOps6::Layout& L) : Name(Name), L(L) {}
        const char* Engine() const override { return Name; }
        bool ReadsOwnBones() const override { return true; }
        const char* GetExportFolderName() const override { return "black_ops_6"; }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            LayoutScope S(L);
            return GameBlackOps6::ReadXModel(Model);
        }
        std::unique_ptr<XImageDDS> ReadXImage(const CoDImage_t* Image) const override
        {
            LayoutScope S(L);
            return GameBlackOps6::ReadXImage(Image);
        }
        std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound) const override
        {
            LayoutScope S(L);
            return GameBlackOps6::ReadXSound(Sound);
        }
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            LayoutScope S(L);
            return GameBlackOps6::ReadXAnim(Animation);
        }
        void LoadStreamedXModel(const std::unique_ptr<XModel_t>& Model, const XModelLod_t& ModelLOD,
                                const std::unique_ptr<WraithModel>& ResultModel) const override
        {
            LayoutScope S(L);
            GameBlackOps6::LoadXModel(Model, ModelLOD, ResultModel);
        }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits64; }

    private:
        const char* Name;
        const GameBlackOps6::Layout& L;
    };

    const GameBlackOps6Live Game("BO6", GameBlackOps6::Multiplayer), Campaign("BO6SP", GameBlackOps6::Campaign);
    const LiveGameRegistration Registered(Game), RegisteredCampaign(Campaign);
}
