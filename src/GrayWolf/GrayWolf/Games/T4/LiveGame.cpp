#include "stdafx.h"

// World at War's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameWorldAtWar.h"
#include "GameWorldWar2.h"
#include "IWDSupport.h"

namespace
{
    class GameWorldAtWarLive final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "T4"; }
        std::vector<PackageMark> Packages() const override { return { { PackageMark::Container::Iwd } }; }
        bool SoundsAsRead() const override { return true; }
        const char* GetExportFolderName() const override { return "world_at_war"; }
        std::unique_ptr<XImageDDS> ReadFileImage(const CoDImage_t* Image) const override
        {
            return IWDSupport::ReadImageFile(Image);
        }
        // the disk path's readers (Games/T4/DiskGame.cpp, over a relocated zone)
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameWorldAtWar::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameWorldAtWar::ReadXModel(Model);
        }
        std::unique_ptr<XSound> ReadXSound(const CoDSound_t* Sound) const override
        {
            return GameWorldWar2::ReadXSound(Sound);
        }
        ViewmodelBoneConvention GetViewmodelBoneConvention() const override { return ViewmodelBoneConvention::Standard; }
        DeltaBitness GetDeltaBitness() const override { return DeltaBitness::Bits32; }
        NotetrackFormat GetNotetrackFormat() const override { return NotetrackFormat::Standard; }
        VertexWeightsVariant GetVertexWeightsVariant() const override { return VertexWeightsVariant::A; }
        NormalUnpackVariant GetNormalUnpackVariant() const override { return NormalUnpackVariant::A; }
    };

    const GameWorldAtWarLive Game;
    const LiveGameRegistration Registered(Game);
}
