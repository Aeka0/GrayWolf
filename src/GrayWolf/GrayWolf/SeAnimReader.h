#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

class WraithAnim;

// Reads SEAnim (DTZxPorter's animation container - the format SEAnimExport.cpp writes) for live
// playback. Spec: https://github.com/SE2Dev/SEAnim-Docs. Playback semantics aren't in that spec (it
// only documents the file layout); the ABSOLUTE/RELATIVE/ADDITIVE/DELTA behaviour implemented in
// SkeletalPreview.cpp instead mirrors the reference importer, SE2Dev/io_anim_seanim's
// import_seanim.py (same author as the format). Single-precision only: this codebase's writer never
// sets SEANIM_PRECISION_HIGH.

struct SeAnimVec3 { float x = 0, y = 0, z = 0; };
struct SeAnimQuat { float x = 0, y = 0, z = 0, w = 1; };

// Matches SEAnimAnimationType in SEAnimExport.cpp (and WraithAnimationType) - same numeric values.
enum class SeAnimBoneType : uint8_t { Absolute = 0, Additive = 1, Relative = 2, Delta = 3 };

struct SeAnimKeyVec3 { uint32_t Frame = 0; SeAnimVec3 Value{}; };
struct SeAnimKeyQuat { uint32_t Frame = 0; SeAnimQuat Value{}; };

struct SeAnimBoneTrack
{
    std::string Name;
    bool HasTypeOverride = false;
    SeAnimBoneType TypeOverride = SeAnimBoneType::Absolute;
    std::vector<SeAnimKeyVec3> PosKeys;
    std::vector<SeAnimKeyQuat> RotKeys;
    std::vector<SeAnimKeyVec3> ScaleKeys;
};

struct SeAnimClip
{
    SeAnimBoneType Type = SeAnimBoneType::Absolute;
    bool Looping = false;
    float FrameRate = 30.0f;
    uint32_t FrameCount = 0;
    std::vector<SeAnimBoneTrack> Bones; // file order; matched to a model's skeleton by name
    // name -> the frames it fires on - same shape as WraithAnim::AnimationNotetracks (WraithAnim.h)
    // by convention, not shared code. Only XAnimBinReader's Layer 2 (ReadXAnimBinClip) fills it.
    std::unordered_map<std::string, std::vector<uint32_t>> Notifications;
};

// Reads a .seanim's header, bone tags, modifiers, and per-bone LOC/ROT/SCALE key tracks. Stops once
// notifications are consumed - the non-spec trailing "SEBlend" blend-shape block some exports carry
// is neither read nor needed for a bone-driven preview. Returns false on a bad magic, an unsupported
// double-precision file, or any short read.
bool ReadSeAnim(const std::string& Path, SeAnimClip& Out);

// Converts a clip into a WraithAnim, so XAnimBinReader.cpp's Layer 2 output reaches
// SEAnim::ExportSEAnim (which only takes a WraithAnim): a field-for-field copy (WraithAnimationType
// and SeAnimBoneType share numeric values). Returns false if the clip has no bone tracks.
bool BuildWraithAnimFromSeAnimClip(const SeAnimClip& Clip, WraithAnim& Out);
