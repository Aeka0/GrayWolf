#include "stdafx.h"

// BO3 customizationtable (34) -> a charactercustomizationtable entry and an entry for each of its
// heads (playerhead), body types (playerbodytype) and their body and helmet styles
// (playerbodystyle, playerhelmetstyle), as HydraX's CustomizationTable.cs (github.com/Scobalula/HydraX,
// GPL-3.0), whose structs (Pack 8) give the offsets below: table header 0x28, head 0x30, body
// type 0x1c0, style 0xa8, accent 0x10 (count, asset pointers).

#include "Gdt.h"
#include "Kvp.h"

#include <format>
#include <set>

namespace
{
    constexpr int64_t ImageName = 0xf8;   // an image's name pointer

    const char* const ImpactTypes[] = { "none", "flesh", "zombie", "armor light", "armor heavy", "robot light",
                                        "robot heavy", "robot boss", "power armor", "skeleton", "flesh corpse" };

    std::string Impact(int32_t I)
    {
        return I >= 0 && I < (int32_t)std::size(ImpactTypes) ? ImpactTypes[I] : std::to_string(I);
    }

    // a body or helmet style's accent colours: accentColor<n>OptionsCount and color_<n>_<m>
    void Accents(const Gdt::Memory& M, int64_t St, Gdt::Block& B)
    {
        const int64_t List = M.Follow(St + 0x78);
        const int Count = List < 0 ? 0 : (int)std::min<uint32_t>(M.At<uint32_t>(St + 0x70), 64);
        for (int K = 0; K < Count; K++)
        {
            const int64_t A = List + 0x10 * (int64_t)K;
            const int64_t Options = M.Follow(A + 8);
            const int N = Options < 0 ? 0 : (int)std::min<uint32_t>(M.At<uint32_t>(A), 256);
            B.Keys.emplace_back(std::format("accentColor{}OptionsCount", K + 1), std::to_string(M.At<int32_t>(A)));
            for (int L = 0; L < N; L++)
                B.Keys.emplace_back(std::format("color_{}_{}", K + 1, L + 1), M.AssetField(Options + 8 * (int64_t)L));
        }
    }

    bool Convert(const Gdt::Type&, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out,
                 Gdt::Stats& S)
    {
        std::set<std::string> Written;   // an entry once in the table's output (a style two body types share)
        const auto Add = [&](Gdt::Block&& B) {
            if (!T7Gdt::EntryName(B.Name)) S.Counters["customization_unnamed"]++;
            else if (Written.insert(B.Name).second) Out.push_back(std::move(B));
        };
        const auto Str = [&](int64_t At) { return M.StringField(At); };
        const auto Asset = [&](int64_t At) { return M.AssetField(At); };
        const auto Image = [&](int64_t At) { return M.AssetField(At, ImageName); };
        const auto Int = [&](int64_t At) { return std::to_string(M.At<int32_t>(At)); };

        // table {name 0, body count 8, bodies 0x10, head count 0x18, heads 0x20}
        Gdt::Block Table{ Name, "charactercustomizationtable.gdf", "", {} };
        const int64_t Bodies = M.Follow(Pos + 0x10), Heads = M.Follow(Pos + 0x20);
        const int BodyCount = Bodies < 0 ? 0 : (int)std::min<uint32_t>(M.At<uint32_t>(Pos + 8), 256);
        const int HeadCount = Heads < 0 ? 0 : (int)std::min<uint32_t>(M.At<uint32_t>(Pos + 0x18), 256);
        Table.Keys.emplace_back("bodyTypeCount", Int(Pos + 8));
        Table.Keys.emplace_back("headCount", Int(Pos + 0x18));

        for (int I = 0; I < HeadCount; I++)
        {
            const int64_t H = Heads + 0x30 * (int64_t)I;
            Gdt::Block B{ Str(H), "playerhead.gdf", "", {} };
            B.Keys.emplace_back("displayName", Str(H + 8));
            B.Keys.emplace_back("iconImage", Image(H + 0x10));
            B.Keys.emplace_back("model", Asset(H + 0x18));
            B.Keys.emplace_back("gender", Int(H + 0x20));
            Table.Keys.emplace_back(std::format("head{:02}", I + 1), B.Name);
            Add(std::move(B));
        }

        for (int I = 0; I < BodyCount; I++)
        {
            const int64_t Bt = Bodies + 0x1c0 * (int64_t)I;
            Gdt::Block B{ Str(Bt), "playerbodytype.gdf", "", {} };
            auto& K = B.Keys;
            K.emplace_back("abilityCardBackDesc", Str(Bt + 0x1a8));
            K.emplace_back("abilityCardBackIcon", Str(Bt + 0x198));
            K.emplace_back("abilityCardBackSubIcon", Str(Bt + 0x1a0));
            K.emplace_back("abilityIconEquipped", Str(Bt + 0x88));
            K.emplace_back("abilityIconUnequipped", Str(Bt + 0x98));
            K.emplace_back("abilitySchema", Str(Bt + 0x1b8));
            K.emplace_back("abilitySubItemDesc", Str(Bt + 0x1b0));
            K.emplace_back("age", Int(Bt + 0x158));
            K.emplace_back("background", Str(Bt + 0x40));
            K.emplace_back("backgroundWithCharacter", Str(Bt + 0x48));
            K.emplace_back("bio", Str(Bt + 0x168));
            K.emplace_back("bodySoundContext", Str(Bt + 0x28));
            K.emplace_back("bodyStyleCount", Int(Bt + 0xa8));
            K.emplace_back("cardBackIcon", Str(Bt + 0x148));
            K.emplace_back("characterFootstepsNPCLoud", Asset(Bt + 0x128));
            K.emplace_back("characterFootstepsNPC", Asset(Bt + 0x120));
            K.emplace_back("characterFootstepsNPCQuiet", Asset(Bt + 0x130));
            K.emplace_back("characterFootsteps", Asset(Bt + 0x110));
            K.emplace_back("characterFootstepsQuiet", Asset(Bt + 0x118));
            K.emplace_back("characterMovementFx", Asset(Bt + 0x108));
            K.emplace_back("characterMovementSounds", Asset(Bt + 0x100));
            K.emplace_back("defaultHeroRenderAbility", Image(Bt + 0x78));
            K.emplace_back("defaultHeroRender", Image(Bt + 0x70));
            K.emplace_back("description", Str(Bt + 0x10));
            K.emplace_back("disabled", M.At<uint8_t>(Bt + 0xcc) ? "1" : "0");
            K.emplace_back("displayName", Str(Bt + 8));
            K.emplace_back("dogtagenemy", Asset(Bt + 0x140));
            K.emplace_back("dogtagfriendly", Asset(Bt + 0x138));
            K.emplace_back("frontendVignetteStruct", Str(Bt + 0xd0));
            K.emplace_back("frontendVignetteWeaponModel", Str(Bt + 0xe8));
            K.emplace_back("frontendVignetteXAnim", Str(Bt + 0xe0));
            K.emplace_back("frontendVignetteXCam", Asset(Bt + 0xd8));
            K.emplace_back("frozenMomentOverlay", Asset(Bt + 0x68));
            K.emplace_back("frozenMomentRender", Asset(Bt + 0x60));
            K.emplace_back("gender", Int(Bt + 0xc8));
            K.emplace_back("genderString", Str(Bt + 0x160));
            K.emplace_back("helmetStyleCount", Int(Bt + 0xb8));
            K.emplace_back("heroAbility", Str(Bt + 0x20));
            K.emplace_back("heroWeapon", Str(Bt + 0x18));
            K.emplace_back("lockedImage", Str(Bt + 0x50));
            K.emplace_back("mpDialog", Asset(Bt + 0x30));
            K.emplace_back("personalizeRender", Str(Bt + 0x58));
            K.emplace_back("realName", Str(Bt + 0x150));
            K.emplace_back("rewardIcon", Str(Bt + 0x38));
            K.emplace_back("weaponCardBackDesc", Str(Bt + 0x180));
            K.emplace_back("weaponCardBackIcon", Str(Bt + 0x170));
            K.emplace_back("weaponCardBackSubIcon", Str(Bt + 0x178));
            K.emplace_back("weaponIconEquipped", Str(Bt + 0x80));
            K.emplace_back("weaponIconUnequipped", Str(Bt + 0x90));
            K.emplace_back("weaponSchema", Str(Bt + 0x190));
            K.emplace_back("weaponSubItemDesc", Str(Bt + 0x188));
            K.emplace_back("zombiePlayerIcon", Image(Bt + 0xa0));

            // body styles (count 0xa8, list 0xb0), then helmet styles (0xb8, 0xc0): entries of their own
            for (int Helmet = 0; Helmet < 2; Helmet++)
            {
                const int64_t List = M.Follow(Bt + (Helmet ? 0xc0 : 0xb0));
                const int Count = List < 0 ? 0 : (int)std::min<uint32_t>(M.At<uint32_t>(Bt + (Helmet ? 0xb8 : 0xa8)), 256);
                for (int J = 0; J < Count; J++)
                {
                    const int64_t St = List + 0xa8 * (int64_t)J;
                    Gdt::Block Sb{ Str(St), Helmet ? "playerhelmetstyle.gdf" : "playerbodystyle.gdf", "", {} };
                    auto& L = Sb.Keys;
                    L.emplace_back("accentColorCount", Int(St + 0x70));
                    if (!Helmet)
                    {
                        L.emplace_back("characterMovementFxOverride", Asset(St + 0x90));
                        L.emplace_back("firstPersonCinematicModel", Asset(St + 0x30));
                        L.emplace_back("firstPersonLegsModel", Asset(St + 0x28));
                        L.emplace_back("gibDef", Asset(St + 0x88));
                        L.emplace_back("gibLegsBoth", Asset(St + 0x68));
                        L.emplace_back("gibLegsClean", Asset(St + 0x50));
                        L.emplace_back("gibLegsLeft", Asset(St + 0x60));
                        L.emplace_back("gibLegsRight", Asset(St + 0x58));
                        L.emplace_back("gibTorsoClean", Asset(St + 0x38));
                        L.emplace_back("gibTorsoLeft", Asset(St + 0x48));
                        L.emplace_back("gibTorsoRight", Asset(St + 0x40));
                        L.emplace_back("viewArmsModel", Asset(St + 0x20));
                    }
                    L.emplace_back("displayName", Str(St + 8));
                    L.emplace_back("iconImage", Image(St + 0x10));
                    L.emplace_back("impactTypeCorpse", Impact(M.At<int32_t>(St + 0x84)));
                    L.emplace_back("impactType", Impact(M.At<int32_t>(St + 0x80)));
                    L.emplace_back("xmodel", Asset(St + 0x18));
                    Accents(M, St, Sb);
                    K.emplace_back(std::format("{}Style{:02}", Helmet ? "helmet" : "body", J + 1), Sb.Name);
                    S.Counters[Helmet ? "customization_helmetstyles" : "customization_bodystyles"]++;
                    Add(std::move(Sb));
                }
            }

            // key/values: count 0xf0, list 0xf8
            std::vector<std::pair<std::string, std::string>> Kv;
            T7Gdt::KeyValues(M, M.Follow(Bt + 0xf8), M.At<uint32_t>(Bt + 0xf0), true, Kv);
            for (auto& [Key, Value] : Kv)
            {
                // HydraX: the Mod Tools name it so though the game types it an xstring
                if (Key.find("challengeSpecialist") != std::string::npos) Key = "streamedimage_challengeSpecialist";
                K.emplace_back(std::move(Key), std::move(Value));
            }
            T7Gdt::Once(K);
            Table.Keys.emplace_back(std::format("bodyType{:02}", I + 1), B.Name);
            S.Counters["customization_bodytypes"]++;
            Add(std::move(B));
        }
        S.Counters["customization_heads"] += HeadCount;
        Out.push_back(std::move(Table));
        return true;
    }
}

GDT_CONVERTER(t7_customizationtable, Convert);
