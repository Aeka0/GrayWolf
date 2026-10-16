#include "stdafx.h"

#include "SEAnimExport.h"

#include "BinaryWriter.h"
#include "WraithAnim.h"

// Specifies how the data is interpreted by the importer
enum class SEAnimAnimationType : uint8_t
{
    // Animation translations are set to this exact value each frame
    SEANIM_ABSOLUTE = 0,
    // This animation is applied to existing animation data in the scene
    SEANIM_ADDITIVE = 1,
    // Animation translations are based on rest position in scene
    SEANIM_RELATIVE = 2,
    // This animation is relative and contains delta data (Whole model movement) Delta tag name must be set!
    SEANIM_DELTA = 3
};

// Specifies the data present for each frame of every bone (Internal use only, matches specification v1.0.1)
enum class SEAnimDataPresenceFlags : uint8_t
{
    // These describe what type of keyframe data is present for the bones
    SEANIM_BONE_LOC = 1 << 0,
    SEANIM_BONE_ROT = 1 << 1,
    SEANIM_BONE_SCALE = 1 << 2,

    // If any of the above flags are set, then bone keyframe data is present, thus this comparing against this mask will return true
    SEANIM_PRESENCE_BONE = SEANIM_BONE_LOC | SEANIM_BONE_ROT | SEANIM_BONE_SCALE,

    // The file contains notetrack data
    SEANIM_PRESENCE_NOTE = 1 << 6,
    // The file contains a custom data block
    SEANIM_PRESENCE_CUSTOM = 1 << 7,
};


namespace
{
    // A frame index or key count, as wide as the frame count needs (1, 2 or 4 bytes)
    void WriteFrameSized(BinaryWriter& Writer, uint32_t FrameCount, uint32_t Value)
    {
        if (FrameCount <= 0xFF)
            Writer.Write<uint8_t>((uint8_t)Value);
        else if (FrameCount <= 0xFFFF)
            Writer.Write<uint16_t>((uint16_t)Value);
        else
            Writer.Write<uint32_t>((uint32_t)Value);
    }

    // One bone's keys of one kind (none written when the animation has no key of that kind):
    // their count, then each frame and value
    template <typename TKeys>
    void WriteBoneKeys(BinaryWriter& Writer, const TKeys& Keys, const std::string& Tag, uint32_t FrameCount)
    {
        if (Keys.size() == 0)
            return;

        const auto Found = Keys.find(Tag);
        if (Found == Keys.end())
        {
            WriteFrameSized(Writer, FrameCount, 0);
            return;
        }

        WriteFrameSized(Writer, FrameCount, (uint32_t)Found->second.size());
        for (auto& Key : Found->second)
        {
            WriteFrameSized(Writer, FrameCount, (uint32_t)Key.Frame);
            Writer.Write(Key.Value);
        }
    }
}

void SEAnim::ExportSEAnim(const WraithAnim& Animation, const std::string& FileName)
{
    auto Writer = BinaryWriter();
    Writer.Create(FileName);
    char Magic[6] = { 'S', 'E', 'A', 'n', 'i', 'm' };
    Writer.Write(Magic);
    Writer.Write<uint16_t>(0x1);
    Writer.Write<uint16_t>(0x1C);
    switch (Animation.AnimType)
    {
    case WraithAnimationType::Absolute:
        Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_ABSOLUTE);
        break;
    case WraithAnimationType::Relative:
        Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_RELATIVE);
        break;
    case WraithAnimationType::Additive:
        Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_ADDITIVE);
        break;
    case WraithAnimationType::Delta:
        Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_DELTA);
        break;
    }
    // Write flags (Looped is the only flag for now)
    Writer.Write<uint8_t>(Animation.Looping ? (1 << 0) : 0);
    {
        uint8_t DataPresentFlags = 0x0;
        if (Animation.AnimationPositionKeys.size() > 0)
        {
            DataPresentFlags |= (uint8_t)SEAnimDataPresenceFlags::SEANIM_BONE_LOC;
        }
        if (Animation.AnimationRotationKeys.size() > 0)
        {
            DataPresentFlags |= (uint8_t)SEAnimDataPresenceFlags::SEANIM_BONE_ROT;
        }
        if (Animation.AnimationScaleKeys.size() > 0)
        {
            DataPresentFlags |= (uint8_t)SEAnimDataPresenceFlags::SEANIM_BONE_SCALE;
        }
        if (Animation.AnimationNotetracks.size() > 0)
        {
            DataPresentFlags |= (uint8_t)SEAnimDataPresenceFlags::SEANIM_PRESENCE_NOTE;
        }
        Writer.Write<uint8_t>(DataPresentFlags);
    }
    // Write data property flags (Precision is the only one for now)
    Writer.Write<uint8_t>(0);
    Writer.Write<uint16_t>(0);
    Writer.Write<float>(Animation.FrameRate);
    uint32_t FrameCountBuffer = Animation.FrameCount();
    uint32_t BoneCountBuffer = Animation.BoneCount();
    uint32_t NotificationBuffer = Animation.NotificationCount();
    Writer.Write<uint32_t>(FrameCountBuffer);
    Writer.Write<uint32_t>(BoneCountBuffer);
    Writer.Write<uint8_t>((uint8_t)Animation.AnimationBoneModifiers.size());
    // Write 3 reserved bytes
    uint8_t Reserved[3] = { 0x0, 0x0, 0x0 };
    Writer.Write(Reserved);
    Writer.Write<uint32_t>(NotificationBuffer);
    {
        auto BoneTags = Animation.Bones();
        for (auto& Tag : BoneTags)
        {
            Writer.WriteNullTerminatedString(Tag);
        }
        for (auto& Modifier : Animation.AnimationBoneModifiers)
        {
            int16_t Index = (uint16_t)std::distance(BoneTags.begin(), BoneTags.find(Modifier.first));
            if (BoneCountBuffer <= 0xFF)
            {
                Writer.Write<uint8_t>((uint8_t)Index);
            }
            else
            {
                Writer.Write<uint16_t>((uint16_t)Index);
            }
            switch (Modifier.second)
            {
            case WraithAnimationType::Absolute:
                Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_ABSOLUTE);
                break;
            case WraithAnimationType::Relative:
                Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_RELATIVE);
                break;
            case WraithAnimationType::Additive:
                Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_ADDITIVE);
                break;
            case WraithAnimationType::Delta:
                Writer.Write<uint8_t>((uint8_t)SEAnimAnimationType::SEANIM_DELTA);
                break;
            }
        }
        // We must write the key info in the order of BoneTags
        for (auto& Tag : BoneTags)
        {
            // Write bone flags, 0 for now
            Writer.Write<uint8_t>(0x0);

            WriteBoneKeys(Writer, Animation.AnimationPositionKeys, Tag, FrameCountBuffer);
            WriteBoneKeys(Writer, Animation.AnimationRotationKeys, Tag, FrameCountBuffer);
            WriteBoneKeys(Writer, Animation.AnimationScaleKeys, Tag, FrameCountBuffer);
        }
    }
    if (NotificationBuffer > 0)
    {
        for (auto& Note : Animation.AnimationNotetracks)
        {
            for (auto& Key : Note.second)
            {
                WriteFrameSized(Writer, FrameCountBuffer, Key);
                Writer.WriteNullTerminatedString(Note.first);
            }
        }
    }

    Writer.Write<uint64_t>(0xA646E656C424553);
    Writer.Write<uint64_t>(Animation.AnimationBlendShapeWeightKeys.size());

    for (auto& BlendShape : Animation.AnimationBlendShapeWeightKeys)
    {
        Writer.WriteNullTerminatedString(BlendShape.first);
    }

    for (auto& BlendShape : Animation.AnimationBlendShapeWeightKeys)
    {
        Writer.Write<uint32_t>((uint32_t)BlendShape.second.size());

        for (auto& Weight : BlendShape.second)
        {
            Writer.Write<uint32_t>(Weight.Frame);
            Writer.Write<Vector3>(Weight.Value);
        }
    }
}