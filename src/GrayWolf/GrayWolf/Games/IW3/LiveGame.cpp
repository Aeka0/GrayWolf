#include "stdafx.h"

// Call of Duty 4: Modern Warfare's live readers, as the common code reaches them (ICoDGame.h).

#include "ICoDGame.h"
#include "GameModernWarfare.h"
#include "GameWorldWar2.h"
#include "IWDSupport.h"

namespace
{
    class GameModernWarfareLive final : public ICoDGame
    {
    public:
        const char* Engine() const override { return "IW3"; }
        std::vector<PackageMark> Packages() const override { return { { PackageMark::Container::Iwd, 0, 0, "call of duty 4" } }; }
        const char* GetExportFolderName() const override { return "modern_warfare"; }
        std::unique_ptr<XImageDDS> ReadFileImage(const CoDImage_t* Image) const override
        {
            return IWDSupport::ReadImageFile(Image);
        }
        // the disk path's readers (Games/IW3/DiskGame.cpp, over a relocated zone)
        std::unique_ptr<XAnim_t> ReadXAnim(const CoDAnim_t* Animation) const override
        {
            return GameModernWarfare::ReadXAnim(Animation);
        }
        std::unique_ptr<XModel_t> ReadXModel(const CoDModel_t* Model) const override
        {
            return GameModernWarfare::ReadXModel(Model);
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

    const GameModernWarfareLive Game;
    const LiveGameRegistration Registered(Game);
}
