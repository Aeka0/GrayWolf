#include "stdafx.h"

#include "TreyarchEffects.h"

#include "ExportNaming.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstring>
#include <iterator>

namespace Treyarch
{
namespace
{
    using Json = nlohmann::ordered_json;

    // ---- the structs, as Greyhound's CoDEffectTranslator read them from the games' memory
#pragma pack(push, 1)
    struct FxIntRange { int32_t Base, Amplitude; };
    struct FxFloatRange { float Base, Amplitude; };
    struct FxElemAtlas
    {
        uint8_t Behavior, Index, Fps, LoopCount, ColIndexBits, RowIndexBits;
        uint16_t EntryCountAndIndexRange;
    };

    // Black Ops 1 and 2 (0x124, 32-bit pointers)
    struct Bo1Elem
    {
        uint32_t Flags;
        FxIntRange Spawn;
        FxFloatRange SpawnRange, FadeInRange, FadeOutRange;
        float SpawnFrustumCullRadius;
        FxIntRange SpawnDelayMsec, LifeSpanMsec;
        FxFloatRange SpawnOrigin[3], SpawnOffsetRadius, SpawnOffsetHeight, SpawnAngles[3], AngularVelocity[3], InitialRotation;
        uint32_t RotationAxis;
        FxFloatRange Gravity, ReflectionFactor;
        FxElemAtlas Atlas;
        float WindInfluence;
        uint8_t ElemType, VisualCount, VelIntervalCount, VisStateIntervalCount;
        uint32_t VelSamples, VisSamples, Visuals;
        float CollMins[3], CollMaxs[3];
        uint32_t EffectOnImpact, EffectOnDeath, EffectEmitted;
        FxFloatRange EmitDist, EmitDistVariance;
        uint32_t EffectAttached, TrailDef;
        uint8_t SortOrder, LightingFrac, Unused[2];
        uint16_t AlphaFadeTimeMsec, MaxWindStrength, SpawnIntervalAtMaxWind, LifespanAtMaxWind;
        FxFloatRange Billboard;   // trim, or for a cloud the density (an int range)
        uint32_t SpawnSound;
        float BillboardPivot[2];
    };
    static_assert(sizeof(Bo1Elem) == 0x124, "BO1/BO2 FxElemDef is 0x124");

    // Black Ops 3 (0x260)
    struct Bo3Elem
    {
        uint32_t Flags, ExtraFlags;
        FxIntRange Spawn, SpawnLoopingSpawnCount;
        FxFloatRange SpawnRange, FadeInRange, FadeOutRange;
        float SpawnFrustumCullRadius;
        FxIntRange SpawnDelayMsec, LifeSpanMsec;
        FxFloatRange SpawnOrigin[3], SpawnOffsetRadius, SpawnOffsetHeight;
        float SpawnOffsetCylindricalAxis;
        FxFloatRange SpawnAngles[3], AngularVelocity[3], InitialRotation;
        uint32_t RotationAxis;
        FxFloatRange Gravity, ReflectionFactor;
        FxElemAtlas Atlas;
        float WindInfluence;
        uint8_t ElemType, VisualCount, VelIntervalCount, ParentMovementStateIntervalCount, AttractorStateIntervalCount,
            VisStateIntervalCount;
        uint16_t Padding;
        uint64_t VelSamples, ParentMovementSamples, VisSamples, AttractorSamples;
        uint64_t Visuals;   // or an inline lens flare / beam source name (0x40)
        uint8_t VisualsRest[0x38];
        uint8_t Unused1[0x40];
        float CollMins[3], CollMaxs[3];
        float AttractorLocalPosition[3];
        uint8_t Unused2[4];
        uint64_t EffectOnImpact, EffectOnDeath, EffectEmitted;
        FxFloatRange EmitDist, EmitDistVariance, EmitDensity;
        float EmitSizeForDensity;
        uint32_t Pad1;
        uint64_t EffectAttached;
        FxFloatRange AttachmentDensity;
        float AttachmentSizeForDensity;
        uint32_t Pad2;
        uint64_t Lighting;   // lens flare, spot light or trail def
        uint8_t Displacement, LightingFrac, Unused[2];
        uint16_t AlphaFadeTimeMsec, MaxWindStrength, SpawnIntervalAtMaxWind, LifespanAtMaxWind;
        FxFloatRange Billboard;
        uint32_t Pad3;
        uint64_t SpawnSound, SpawnSoundHash, FollowSound, FollowSoundHash;
        float BillboardPivot[2];
        float AlphaDissolve, ZFeather;
        uint32_t FallOffBeginAngle, FallOffEndAngle;
        uint8_t Unmapped[0x20];
    };
    static_assert(sizeof(Bo3Elem) == 0x260, "BO3 FxElemDef is 0x260");
#pragma pack(pop)
    // where the zonekit tables (t6 f18b4a0, t7 f13fb7d0) read the element's pointers
    static_assert(offsetof(Bo1Elem, ElemType) == 0xb8 && offsetof(Bo1Elem, VelSamples) == 0xbc &&
                  offsetof(Bo1Elem, Visuals) == 0xc4 && offsetof(Bo1Elem, EffectOnImpact) == 0xe0 &&
                  offsetof(Bo1Elem, EffectAttached) == 0xfc && offsetof(Bo1Elem, SpawnSound) == 0x118, "BO1/BO2 element");
    static_assert(offsetof(Bo3Elem, ElemType) == 0xc8 && offsetof(Bo3Elem, VelSamples) == 0xd0 &&
                  offsetof(Bo3Elem, VisSamples) == 0xe0 && offsetof(Bo3Elem, Visuals) == 0xf0 &&
                  offsetof(Bo3Elem, EffectOnImpact) == 0x198 && offsetof(Bo3Elem, EffectAttached) == 0x1d0 &&
                  offsetof(Bo3Elem, Lighting) == 0x1e8 && offsetof(Bo3Elem, SpawnSound) == 0x208 &&
                  offsetof(Bo3Elem, FollowSound) == 0x218, "BO3 element");

    const char* const Bo1Types[] = { "billboardSprite", "orientedSprite", "rotatedSprite", "tail", "line", "trail", "cloud",
                                     "model", "omniLight", "spotLight", "sound", "decal", "runner" };
    const char* const Bo3Types[] = { "billboardSprite", "orientedSprite", "rotatedSprite", "tail", "line", "trail",
                                     "cloud", "model", "dynamicLight", "omniLight", "spotLight", "dynamicSound",
                                     "lensFlare", "decal", "runner", "beamSource", "beamTarget" };

    // the single-bit flags, by bit (the masked ones - spawn offset, run, orientation - apart)
    struct Bit { uint32_t Mask; const char* Name; };
    const Bit Bo1Flags[] = { { 0x2, "spawnRelative" }, { 0x4, "spawnFrustumCull" }, { 0x8, "runnerUsesRandRot" },
                             { 0x100, "useCollision" }, { 0x200, "dieOnTouch" }, { 0x400, "drawPastFog" },
                             { 0x800, "drawWithViewModel" }, { 0x1000, "blocksSight" }, { 0x2000, "useItemClip" },
                             { 0x80000, "useWorldUp" }, { 0x100000, "alignViewpoint" }, { 0x200000, "useBillboardPivot" },
                             { 0x400000, "useGaussianCloud" }, { 0x800000, "useRotationAxis" },
                             { 0x1000000, "hasVelocityGraphLocal" }, { 0x2000000, "hasVelocityGraphWorld" },
                             { 0x4000000, "hasGravity" }, { 0x8000000, "modelUsesPhysics" }, { 0x10000000, "nonUniformScale" },
                             { 0x20000000, "flameChunk" }, { 0x40000000, "hasReflection" }, { 0x80000000, "isMatureContent" } };
    const Bit Bo3Flags[] = { { 0x2, "spawnRelative" }, { 0x4, "spawnFrustumCull" }, { 0x8, "runnerUsesRandRot" },
                             { 0x200, "useCollision" }, { 0x400, "dieOnTouch" }, { 0x800, "drawPastFog" },
                             { 0x1000, "drawWithViewModel" }, { 0x2000, "blocksSight" }, { 0x4000, "useItemClip" },
                             { 0x8000, "useOcclusionQuery" }, { 0x10000, "inheritParentMovement" }, { 0x20000, "useThermal" },
                             { 0x40000, "attractorGraphEnable" }, { 0x80000, "useWorldUp" }, { 0x100000, "alignViewpoint" },
                             { 0x200000, "useBillboardPivot" }, { 0x400000, "useGaussianCloud" }, { 0x800000, "useRotationAxis" },
                             { 0x8000000, "modelUsesPhysics" }, { 0x10000000, "nonUniformScale" },
                             { 0x20000000, "noComputeSprites" }, { 0x40000000, "spawnDynEnt" }, { 0x80000000, "isMatureContent" } };
    const Bit Bo3Extra[] = { { 0x1, "distribX" }, { 0x2, "distribY" }, { 0x4, "distribZ" }, { 0x8, "teamFriendly" },
                             { 0x10, "teamFoe" }, { 0x20, "castShadow" }, { 0x40, "underwaterOnly" }, { 0x80, "overwaterOnly" },
                             { 0x100, "waterRelaxedCull" }, { 0x400, "gameplayIntensive" },
                             { 0x800, "useSizeBasedDensityEmission" }, { 0x1000, "useSizeBasedDensityAttachment" },
                             { 0x2000, "lfOccludeGeometry" }, { 0x4000, "lfOccludeParticles" },
                             { 0x8000, "respectExclusionVolumes" }, { 0x10000, "noGpuFx" }, { 0x20000, "indoor" },
                             { 0x80000, "useGlobalTint" }, { 0x100000, "useTrailTrapezoidFix" } };

    template <size_t N>
    Json Bits(uint32_t V, const Bit (&Table)[N])
    {
        Json Out = Json::array();
        for (const Bit& B : Table)
            if (V & B.Mask) Out.push_back(B.Name);
        return Out;
    }

    Json Range(const FxFloatRange& R) { return Json::array({ R.Base, R.Amplitude }); }
    Json Range(const FxIntRange& R) { return Json::array({ R.Base, R.Amplitude }); }
    Json Ranges(const FxFloatRange* R) { return Json::array({ Range(R[0]), Range(R[1]), Range(R[2]) }); }
    Json Vec(const float* V, int N)
    {
        Json Out = Json::array();
        for (int I = 0; I < N; I++) Out.push_back(V[I]);
        return Out;
    }

    class Reader
    {
    public:
        Reader(const EffectZone& Z, EffectLayout L) : Z(Z), Ptr(L == EffectLayout::Bo1 || L == EffectLayout::Bo2 ? 4 : 8) {}

        template <class T>
        bool Struct(uint64_t Address, T& Out) const
        {
            const int64_t O = Z.Off(Address);
            if (O < 0 || O + (int64_t)sizeof(T) > (int64_t)Z.Image->size()) return false;
            std::memcpy(&Out, Z.Image->data() + O, sizeof(T));
            return true;
        }
        uint64_t PointerAt(int64_t Pos) const
        {
            return Ptr == 8 ? ExportNaming::At<uint64_t>(*Z.Image, Pos) : ExportNaming::At<uint32_t>(*Z.Image, Pos);
        }
        // the asset an address points at, by the name its first field points to (a name-only
        // header's ",name" as name)
        std::string Asset(uint64_t Address) const
        {
            const int64_t O = Z.Off(Address);
            if (O < 0) return std::string();
            if (Z.AssetName) return Z.AssetName(O);
            std::string N = Z.String(PointerAt(O));
            if (!N.empty() && N[0] == ',') N.erase(0, 1);
            return N;
        }
        // a sound or an effect: its name where the zone holds the pointer as the name's text (an
        // effect handle, an alias), else the asset it points at
        std::string Named(uint64_t Address) const
        {
            std::string S = Z.String(Address);
            bool Text = S.size() >= 3;
            for (char C : S)
                Text = Text && ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '_' ||
                                C == '/' || C == '\\' || C == '.' || C == '-');
            return Text ? S : Asset(Address);
        }
        std::string Text(const uint8_t* Bytes, size_t Max) const
        {
            size_t N = 0;
            while (N < Max && Bytes[N]) N++;
            return std::string(reinterpret_cast<const char*>(Bytes), N);
        }

        const EffectZone& Z;
        const int Ptr;
    };

    // What an element of a type draws or plays, by name: count 1 is the pointer itself, more an
    // array of them (BO3: 0x40 per visual)
    Json Visuals(const Reader& R, EffectLayout L, uint8_t Type, uint8_t Count, uint64_t Visuals, const uint8_t* Inline)
    {
        const bool Bo3 = L == EffectLayout::Bo3;
        const char* Key = "material";
        if (Bo3)
        {
            switch (Type)
            {
            case 7: Key = "model"; break;
            case 11: Key = "sound"; break;
            case 13: Key = "decal"; break;
            case 14: Key = "effect"; break;
            case 15: return Json::array({ Json{ { "beam", R.Text(Inline, 0x40) } } });
            case 8: case 9: case 10: case 12: case 16: return Json::array();
            default: if (Type > 6) return Json::array();
            }
        }
        else
        {
            switch (Type)
            {
            case 7: Key = "model"; break;
            case 10: Key = "sound"; break;
            case 11: Key = "decal"; break;
            case 12: Key = "effect"; break;
            case 8: case 9: return Json::array();
            default: if (Type > 6) return Json::array();
            }
        }
        Json Out = Json::array();
        const std::string K = Key;
        const auto Named = [&](uint64_t A) { return K == "sound" || K == "effect" ? R.Named(A) : R.Asset(A); };
        // a decal's always a list of mark visuals, each its materials (BO3 three, BO1/2 two): the
        // first of each named
        if (K == "decal")
        {
            const int64_t Array = R.Z.Off(Visuals);
            const int64_t Stride = Bo3 ? 0x18 : 2 * R.Ptr;
            for (int I = 0; Array >= 0 && I < Count; I++)
                if (std::string N = R.Asset(R.PointerAt(Array + I * Stride)); !N.empty()) Out.push_back(Json{ { "material", N } });
            return Out;
        }
        if (Count == 1)
        {
            if (std::string N = Named(Visuals); !N.empty()) Out.push_back(Json{ { Key, N } });
            return Out;
        }
        const int64_t Array = R.Z.Off(Visuals);
        const int64_t Stride = Bo3 ? 0x40 : R.Ptr;
        for (int I = 0; Array >= 0 && I < Count; I++)
            if (std::string N = Named(R.PointerAt(Array + I * Stride)); !N.empty()) Out.push_back(Json{ { Key, N } });
        return Out;
    }

    // velocity samples (0x60 each: local velocity and total delta, then world)
    Json VelocitySamples(const Reader& R, uint64_t At, int Count)
    {
        Json Out = Json::array();
        struct Sample { float V[24]; } S{};
        for (int I = 0; I < Count && R.Struct(At + 0x60ull * I, S); I++)
        {
            const auto Part = [&](int K) {
                return Json{ { "velocity", { { "base", Vec(S.V + K, 3) }, { "amplitude", Vec(S.V + K + 3, 3) } } },
                             { "totalDelta", { { "base", Vec(S.V + K + 6, 3) }, { "amplitude", Vec(S.V + K + 9, 3) } } } };
            };
            Out.push_back(Json{ { "local", Part(0) }, { "world", Part(12) } });
        }
        return Out;
    }

    // visual state samples: base then amplitude, each a colour (RGBA bytes), rotation delta and
    // total, size (2), scale, and on BO3 light intensity, radius, fov and child scale (a state of
    // 0x18 bytes, BO3 0x28: the loaders read 48 and 80 bytes a sample)
    Json VisualSamples(const Reader& R, uint64_t At, int Count, bool Bo3)
    {
        Json Out = Json::array();
        const size_t State = Bo3 ? 0x28 : 0x18;
        uint8_t B[0x50] = {};
        for (int I = 0; I < Count; I++)
        {
            const int64_t O = R.Z.Off(At + 2 * State * I);
            if (O < 0 || O + (int64_t)(2 * State) > (int64_t)R.Z.Image->size()) break;
            std::memcpy(B, R.Z.Image->data() + O, 2 * State);
            const auto Part = [&](size_t P) {
                float F[9] = {};
                std::memcpy(F, B + P + 4, State - 4);
                Json J{ { "color", Json::array({ B[P], B[P + 1], B[P + 2], B[P + 3] }) }, { "rotationDelta", F[0] },
                        { "rotationTotal", F[1] }, { "size", Json::array({ F[2], F[3] }) }, { "scale", F[4] } };
                if (Bo3)
                {
                    J["lightIntensity"] = F[5];
                    J["lightRadius"] = F[6];
                    J["lightFov"] = F[7];
                    J["childScale"] = F[8];
                }
                return J;
            };
            Out.push_back(Json{ { "base", Part(0) }, { "amplitude", Part(State) } });
        }
        return Out;
    }

    // what both layouts share, from a struct of either
    template <class E>
    Json Common(const Reader& R, EffectLayout L, const E& E1)
    {
        const bool Bo3 = L == EffectLayout::Bo3;
        Json J;
        const uint8_t Type = E1.ElemType;
        const auto& Names = Bo3 ? Bo3Types : Bo1Types;
        const size_t Known = Bo3 ? std::size(Bo3Types) : std::size(Bo1Types);
        J["type"] = Type < Known ? Json(Names[Type]) : Json(std::to_string(Type));
        J["flags"] = Bo3 ? Bits(E1.Flags, Bo3Flags) : Bits(E1.Flags, Bo1Flags);
        static const char* const Offsets[] = { "none", "sphere", "cylinder", "?" };
        J["spawnOffset"] = Offsets[(E1.Flags >> 4) & 3];
        static const char* const Runs[] = { "world", "spawn", "effect", "offset", "effectNow", "?", "?", "?" };
        J["runRelativeTo"] = Runs[Bo3 ? (E1.Flags >> 6) & 7 : (E1.Flags >> 6) & 3];
        J["spawn"] = Range(E1.Spawn);
        J["spawnRange"] = Range(E1.SpawnRange);
        J["fadeInRange"] = Range(E1.FadeInRange);
        J["fadeOutRange"] = Range(E1.FadeOutRange);
        J["spawnFrustumCullRadius"] = E1.SpawnFrustumCullRadius;
        J["spawnDelayMsec"] = Range(E1.SpawnDelayMsec);
        J["lifeSpanMsec"] = Range(E1.LifeSpanMsec);
        J["spawnOrigin"] = Ranges(E1.SpawnOrigin);
        J["spawnOffsetRadius"] = Range(E1.SpawnOffsetRadius);
        J["spawnOffsetHeight"] = Range(E1.SpawnOffsetHeight);
        J["spawnAngles"] = Ranges(E1.SpawnAngles);
        J["angularVelocity"] = Ranges(E1.AngularVelocity);
        J["initialRotation"] = Range(E1.InitialRotation);
        J["rotationAxisPacked"] = E1.RotationAxis;
        J["gravity"] = Range(E1.Gravity);
        J["elasticity"] = Range(E1.ReflectionFactor);
        J["windInfluence"] = E1.WindInfluence;
        J["atlas"] = Json{ { "behavior", E1.Atlas.Behavior }, { "index", E1.Atlas.Index }, { "fps", E1.Atlas.Fps },
                           { "loopCount", E1.Atlas.LoopCount }, { "colIndexBits", E1.Atlas.ColIndexBits },
                           { "rowIndexBits", E1.Atlas.RowIndexBits },
                           { "entryCount", 1 << ((E1.Atlas.ColIndexBits + E1.Atlas.RowIndexBits) & 15) } };
        J["collision"] = Json{ { "mins", Vec(E1.CollMins, 3) }, { "maxs", Vec(E1.CollMaxs, 3) } };
        J["fxOnImpact"] = R.Named(E1.EffectOnImpact);
        J["fxOnDeath"] = R.Named(E1.EffectOnDeath);
        J["fxEmitted"] = R.Named(E1.EffectEmitted);
        J["emitDist"] = Range(E1.EmitDist);
        J["emitDistVariance"] = Range(E1.EmitDistVariance);
        J["fxAttached"] = R.Named(E1.EffectAttached);
        J["lightingFrac"] = E1.LightingFrac;
        J["alphaFadeTimeMsec"] = E1.AlphaFadeTimeMsec;
        J["maxWindStrength"] = E1.MaxWindStrength;
        J["spawnIntervalAtMaxWind"] = E1.SpawnIntervalAtMaxWind;
        J["lifespanAtMaxWind"] = E1.LifespanAtMaxWind;
        J["billboard"] = Range(E1.Billboard);
        J["billboardPivot"] = Vec(E1.BillboardPivot, 2);
        J["spawnSound"] = E1.SpawnSound ? R.Named(E1.SpawnSound) : std::string();
        J["velocitySamples"] = VelocitySamples(R, E1.VelSamples, E1.VelIntervalCount + 1);
        J["visualSamples"] = VisualSamples(R, E1.VisSamples, E1.VisStateIntervalCount + 1, Bo3);
        return J;
    }
}

namespace
{
    // a Cold War element as far as its tables say: its type (BO3's names, 17 a vector field), its
    // visuals (one asset, a list of 0x40, or a decal's list of material pairs) and four effects
    Json ColdWarElement(const Reader& R, int64_t O)
    {
        using ExportNaming::At;
        const auto& Img = *R.Z.Image;
        const uint8_t Type = At<uint8_t>(Img, O + 0x328), Count = At<uint8_t>(Img, O + 0x332);
        Json E;
        E["type"] = Type < std::size(Bo3Types) ? Json(Bo3Types[Type]) : Type == 17 ? Json("vectorField") : Json(std::to_string(Type));
        const char* Key = Type == 7 ? "model" : Type == 14 ? "effect" : Type == 17 ? "vectorField" : "material";
        const bool Drawn = Type <= 7 || Type == 12 || Type == 13 || Type == 14 || Type == 17;
        Json V = Json::array();
        const uint64_t Visuals = R.PointerAt(O);
        const auto Add = [&](const char* K, uint64_t A) {
            if (std::string N = R.Asset(A); !N.empty()) V.push_back(Json{ { K, N } });
        };
        const int64_t Array = R.Z.Off(Visuals);
        if (Type == 13)
            for (int64_t I = 0; Array >= 0 && I < Count; I++) Add("material", R.PointerAt(Array + I * 0x10));
        else if (Drawn && Count == 1) Add(Key, Visuals);
        else if (Drawn)
            for (int64_t I = 0; Array >= 0 && I < Count; I++) Add(Key, R.PointerAt(Array + I * 0x40));
        E["visuals"] = std::move(V);
        Json Fx = Json::array();
        for (int K = 0; K < 4; K++)
            if (std::string N = R.Asset(R.PointerAt(O + 0x160 + 8 * K)); !N.empty()) Fx.push_back(N);
        E["effects"] = std::move(Fx);
        return E;
    }
}

bool ReadEffect(const EffectZone& Z, EffectLayout L, int64_t Pos, const std::string& Name, std::string& Out)
{
    using ExportNaming::At;
    const Reader R(Z, L);
    const auto& Img = *Z.Image;
    const bool Bo3 = L == EffectLayout::Bo3;
    // the effect: counts, elements, lifetimes, bounds
    uint32_t Counts[3];
    uint64_t Elements;
    Json J;
    J["name"] = Name;
    if (L == EffectLayout::Bo1)
    {
        for (int I = 0; I < 3; I++) Counts[I] = At<uint32_t>(Img, Pos + 0x10 + 4 * I);
        Elements = At<uint32_t>(Img, Pos + 0x1c);
        J["flags"] = At<uint8_t>(Img, Pos + 4);
        J["priority"] = At<uint8_t>(Img, Pos + 5);
        J["loopingLifeMsec"] = At<uint32_t>(Img, Pos + 0xc);
        float B[7];
        for (int I = 0; I < 7; I++) B[I] = At<float>(Img, Pos + 0x20 + 4 * I);
        J["boundingBoxDim"] = Vec(B, 3);
        J["boundingSphere"] = Vec(B + 3, 4);
    }
    else if (L == EffectLayout::ColdWar)
    {
        // fx (0x90): the element counts at 0x1c (summed by the loader), the elements at 0x28
        for (int I = 0; I < 3; I++) Counts[I] = At<uint16_t>(Img, Pos + 0x1c + 2 * I);
        Elements = R.PointerAt(Pos + 0x28);
    }
    else
    {
        const int64_t P = R.Ptr;   // the name pointer's width moves the rest
        for (int I = 0; I < 3; I++) Counts[I] = At<uint16_t>(Img, Pos + P + 4 + 2 * I);
        J["flags"] = At<uint16_t>(Img, Pos + P);
        J["priority"] = At<uint16_t>(Img, Pos + P + 2);
        J["loopingLifeMsec"] = At<uint32_t>(Img, Pos + P + 0x10);
        J["nonLoopingLifeMsec"] = At<uint32_t>(Img, Pos + P + 0x14);
        Elements = R.PointerAt(Pos + P + 0x18);
        float B[6];
        const int64_t Bounds = Pos + P + 0x18 + P;
        for (int I = 0; I < 6; I++) B[I] = At<float>(Img, Bounds + 4 * I);
        J["boundingBoxDim"] = Vec(B, 3);
        J["boundingCenter"] = Vec(B + 3, 3);
    }
    J["counts"] = Json{ { "looping", Counts[0] }, { "oneShot", Counts[1] }, { "emission", Counts[2] } };
    const bool Cw = L == EffectLayout::ColdWar;
    const size_t Size = Cw ? 0x340 : Bo3 ? sizeof(Bo3Elem) : sizeof(Bo1Elem);
    const uint32_t Total = Counts[0] + Counts[1] + Counts[2];
    if (Total > 0 && Z.Off(Elements) < 0) return false;
    Json List = Json::array();
    static const char* const Groups[] = { "looping", "oneShot", "emission" };
    uint32_t Index = 0;
    for (int G = 0; G < 3; G++)
        for (uint32_t I = 0; I < Counts[G]; I++, Index++)
        {
            const uint64_t A = Elements + (uint64_t)Index * Size;
            Json E{ { "group", Groups[G] }, { "index", I } };
            if (Cw)
            {
                const int64_t O = Z.Off(A);
                if (O < 0 || O + (int64_t)Size > (int64_t)Img.size()) return false;
                E.update(ColdWarElement(R, O));
            }
            else if (Bo3)
            {
                Bo3Elem X{};
                if (!R.Struct(A, X)) return false;
                E.update(Common(R, L, X));
                E["extraFlags"] = Bits(X.ExtraFlags, Bo3Extra);
                E["spawnLoopingSpawnCount"] = Range(X.SpawnLoopingSpawnCount);
                E["spawnOffsetCylindricalAxis"] = X.SpawnOffsetCylindricalAxis;
                E["attractorLocalPosition"] = Vec(X.AttractorLocalPosition, 3);
                E["emitDensity"] = Range(X.EmitDensity);
                E["emitSizeForDensity"] = X.EmitSizeForDensity;
                E["attachmentDensity"] = Range(X.AttachmentDensity);
                E["attachmentSizeForDensity"] = X.AttachmentSizeForDensity;
                E["displacement"] = X.Displacement;
                E["followSound"] = X.FollowSound ? R.Named(X.FollowSound) : std::string();
                E["alphaDissolve"] = X.AlphaDissolve;
                E["zFeather"] = X.ZFeather;
                E["falloffAngles"] = Json::array({ X.FallOffBeginAngle, X.FallOffEndAngle });
                E["visuals"] = Visuals(R, L, X.ElemType, X.VisualCount, X.Visuals, reinterpret_cast<const uint8_t*>(&X.Visuals));
            }
            else
            {
                Bo1Elem X{};
                if (!R.Struct(A, X)) return false;
                E.update(Common(R, L, X));
                E["sortOrder"] = X.SortOrder;
                E["visuals"] = Visuals(R, L, X.ElemType, X.VisualCount, X.Visuals, nullptr);
            }
            List.push_back(std::move(E));
        }
    J["elements"] = std::move(List);
    // a name read off bytes that are not UTF-8 is written with U+FFFD, not thrown
    Out = J.dump(1, '\t', false, Json::error_handler_t::replace);
    Out += "\n";
    return true;
}
}
