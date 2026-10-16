#include "stdafx.h"

#include "GameModernWarfare.h"

#include "GameReaders.h"

// These readers serve the disk path (Games/IW3/DiskGame.cpp): a zone walked by the zonekit
// iw3 tables and served relocated as the game's memory (VirtualGameSession).

std::unique_ptr<XAnim_t> GameModernWarfare::ReadXAnim(const CoDAnim_t* Animation)
{
    return GameReaders::ReadAnim<MWXAnim, MWXAnimDeltaParts>(Animation, 2, { "viewmodel_" });
}

std::unique_ptr<XModel_t> GameModernWarfare::ReadXModel(const CoDModel_t* Model)
{
    return GameReaders::ReadModel<MWXModel, MWXModelSurface, uint32_t>(Model, 2, ReadXMaterial);
}

const XMaterial_t GameModernWarfare::ReadXMaterial(uint64_t MaterialPointer)
{
    return GameReaders::ReadMaterial32<MWXMaterial, MWXMaterialImage, MWGfxImage>(MaterialPointer, 0x40);
}

std::unique_ptr<XImageDDS> GameModernWarfare::LoadXImage(const XImage_t& Image)
{
    return GameReaders::LoadIwdImage(Image);
}
