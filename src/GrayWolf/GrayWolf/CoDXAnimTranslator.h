#pragma once

#include <cstdint>
#include <memory>

#include "CoDAssets.h"

// A class that handles translating generic Call of Duty XAnims to Wraith Anims
class CoDXAnimTranslator
{
public:

    static std::unique_ptr<WraithAnim> TranslateXAnim(const std::unique_ptr<XAnim_t>& Animation);

private:


    static void NotetracksStandard(const std::unique_ptr<WraithAnim>& Anim, const std::unique_ptr<XAnim_t>& Animation);
    static void NotetracksBO3(const std::unique_ptr<WraithAnim>& Anim, const std::unique_ptr<XAnim_t>& Animation);
    static void NotetracksBO4(const std::unique_ptr<WraithAnim>& Anim, const std::unique_ptr<XAnim_t>& Animation);
    static void NotetracksCW(const std::unique_ptr<WraithAnim>& Anim, const std::unique_ptr<XAnim_t>& Animation);
    static void NotetracksVG(const std::unique_ptr<WraithAnim>& Anim, const std::unique_ptr<XAnim_t>& Animation);

    static void BlendShapesBO4(const std::unique_ptr<WraithAnim>& Anim, const std::unique_ptr<XAnim_t>& Animation);
    static void BlendShapesMW(const std::unique_ptr<WraithAnim>& Anim, const std::unique_ptr<XAnim_t>& Animation);

    static void DeltaTranslations32(const std::unique_ptr<WraithAnim>& Anim, uint32_t FrameSize, const std::unique_ptr<XAnim_t>& Animation);
    static void DeltaTranslations64(const std::unique_ptr<WraithAnim>& Anim, uint32_t FrameSize, const std::unique_ptr<XAnim_t>& Animation);

    static void Delta2DRotations32(const std::unique_ptr<WraithAnim>& Anim, uint32_t FrameSize, const std::unique_ptr<XAnim_t>& Animation);
    static void Delta2DRotations64(const std::unique_ptr<WraithAnim>& Anim, uint32_t FrameSize, const std::unique_ptr<XAnim_t>& Animation);

    static void Delta3DRotations32(const std::unique_ptr<WraithAnim>& Anim, uint32_t FrameSize, const std::unique_ptr<XAnim_t>& Animation);
    static void Delta3DRotations64(const std::unique_ptr<WraithAnim>& Anim, uint32_t FrameSize, const std::unique_ptr<XAnim_t>& Animation);
};