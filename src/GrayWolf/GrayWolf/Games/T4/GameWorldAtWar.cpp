#include "stdafx.h"

#include "GameWorldAtWar.h"

#include "GameReaders.h"

// These readers serve the disk path (Games/T4/DiskGame.cpp): a zone walked by the zonekit
// t4 tables and served relocated as the game's memory (VirtualGameSession).

std::unique_ptr<XAnim_t> GameWorldAtWar::ReadXAnim(const CoDAnim_t* Animation)
{
    return GameReaders::ReadAnim<WAWXAnim, WAWXAnimDeltaParts>(Animation, 2, { "viewmodel_" });
}

std::unique_ptr<XModel_t> GameWorldAtWar::ReadXModel(const CoDModel_t* Model)
{
    return GameReaders::ReadModel<WAWXModel, WAWXModelSurface, uint32_t>(Model, 2, ReadXMaterial);
}

const XMaterial_t GameWorldAtWar::ReadXMaterial(uint64_t MaterialPointer)
{
    return GameReaders::ReadMaterial32<WAWXMaterial, WAWXMaterialImage, WAWGfxImage>(MaterialPointer, 0x60);
}

std::unique_ptr<XImageDDS> GameWorldAtWar::LoadXImage(const XImage_t& Image)
{
    return GameReaders::LoadIwdImage(Image);
}
