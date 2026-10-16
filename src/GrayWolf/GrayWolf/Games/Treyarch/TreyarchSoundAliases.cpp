#include "stdafx.h"

#include "Games/Treyarch/TreyarchSoundAliases.h"

#include "Games/Treyarch/TreyarchSoundNames.h"

#include <string>
#include <vector>

using SoundAliasCsv::Col;

namespace Treyarch
{
namespace
{
    // OAT's SoundConstantsT6.h (the game's SndDriverGlobals and executable)
    constexpr const char* Groups[] = {
        "grp_reference", "grp_master", "grp_wpn_lfe", "grp_lfe", "grp_hdrfx", "grp_music", "grp_voice", "grp_set_piece",
        "grp_igc", "grp_mp_game", "grp_explosion", "grp_player_impacts", "grp_scripted_moment", "grp_menu", "grp_whizby",
        "grp_weapon", "grp_vehicle", "grp_impacts", "grp_foley", "grp_destructible", "grp_physics", "grp_ambience",
        "grp_alerts", "grp_air", "grp_bink", "grp_announcer",
    };
    constexpr const char* Curves[] = {
        "default", "defaultmin", "allon", "alloff", "rcurve0", "rcurve1", "rcurve2", "rcurve3", "rcurve4", "rcurve5",
        "steep", "sindelay", "cosdelay", "sin", "cos", "rev60", "rev65",
    };
    constexpr const char* DuckGroups[] = {
        "snp_alerts_gameplay", "snp_ambience", "snp_claw", "snp_destructible", "snp_dying", "snp_dying_ice", "snp_evt_2d",
        "snp_explosion", "snp_foley", "snp_grenade", "snp_hdrfx", "snp_igc", "snp_impacts", "snp_menu", "snp_movie",
        "snp_music", "snp_never_duck", "snp_player_dead", "snp_player_impacts", "snp_scripted_moment", "snp_set_piece",
        "snp_special", "snp_vehicle", "snp_vehicle_interior", "snp_voice", "snp_weapon_decay_1p", "snp_whizby",
        "snp_wpn_1p", "snp_wpn_3p", "snp_wpn_turret", "snp_x2", "snp_x3",
    };
    constexpr const char* Pans[] = {
        "default", "music", "wpn_all", "wpn_fnt", "wpn_rear", "wpn_left", "wpn_right", "music_all", "fly_foot_all",
        "front", "back", "front_mostly", "back_mostly", "all", "center", "front_and_center", "lfe", "quad",
        "front_mostly_some_center", "front_halfback", "halffront_back", "test", "brass_right", "brass_left", "veh_back",
        "tst_left", "tst_center", "tst_right", "tst_surround_left", "tst_surround_right", "tst_lfe", "pip", "movie_vo",
    };
    constexpr const char* LimitTypes[] = { "none", "oldest", "reject", "priority" };
    // the mod tools' FluxType names ("random" in the T6 executable is BO3's random_player)
    constexpr const char* MoveTypes[] = { "", "left_player", "center_player", "right_player", "random_player", "left_shot", "center_shot", "right_shot" };
    constexpr const char* LoadTypes[] = { "", "loaded", "streamed", "primed" };
    constexpr const char* Buses[] = { "bus_reverb", "bus_fx", "bus_voice", "bus_pfutz", "bus_hdrfx", "bus_ui", "bus_music", "bus_movie", "bus_reference" };
    // context types and values, futz patches the T6 executable names (OAT's dumper)
    constexpr const char* KnownNames[] = {
        "plr_stance", "grass", "f35", "ringoff_plr", "mature", "stand", "crouch", "prone", "no_grass", "in_grass",
        "interior", "exterior", "outdoor", "indoor", "safe", "explicit", "bfutz", "default", "defaultbkp", "dlc_res_1",
        "dlc_res_2", "dlc_res_3", "dlc_res_4", "dlc_res_5", "dlc_res_6", "dlc_res_7", "dlc_res_8", "good_1",
        "jet_wing_helmet", "jet_wing_helmet_flying", "mpl_agr_pov", "mpl_chopper_pov", "mpl_quad_pov", "mpl_reaper_pov",
        "no_gfutz", "spl_asd_pov", "spl_bigdog_pov", "spl_heli_future", "spl_quad_pov", "spl_spiderbot_pov", "spl_spymic",
        "spl_tow_missile", "spl_turret", "spl_war_command", "test_1", "test_2", "tueyeckert",
    };

    uint32_t Bits(uint32_t V, int From, int N) { return (V >> From) & ((1u << N) - 1); }

    std::string Randomize(uint32_t V)
    {
        // SOUND_RANDOMIZE_TYPES: volume 1, pitch 2, variant 4
        std::string S;
        const char* Names[] = { "volume", "pitch", "variant" };
        for (int K = 0; K < 3; K++)
            if (V & (1u << K)) S += (S.empty() ? "" : " ") + std::string(Names[K]);
        return S;
    }

    // the bank's alias lists: name 0, id 4, head 8, count 0xc (0x14 each)
    template <class F>
    void EachVariant(const SoundAliasCsv::ZoneView& Z, int64_t Bank, int64_t Size, SoundAliasCsv::Rows& Out, F&& Each)
    {
        const uint32_t Count = Z.At<uint32_t>(Bank + 4);
        const int64_t Lists = Z.Follow(Bank + 8);
        if (Count == 0) return;
        if (Lists < 0)
        {
            Out.Counters["alias_lists_unread"] += Count;
            return;
        }
        for (uint32_t L = 0; L < Count; L++)
        {
            const int64_t List = Lists + (int64_t)L * 0x14;
            const int64_t Head = Z.Follow(List + 8);
            const uint32_t N = Z.At<uint32_t>(List + 0xc);
            if (Head < 0 || N > 4096)
            {
                Out.Counters["alias_lists_unread"]++;
                continue;
            }
            Out.Counters["alias_lists"]++;
            const std::string Name = Z.Text(List);
            for (uint32_t K = 0; K < N; K++) Each(Name, Head + (int64_t)K * Size);
        }
    }
}

void ReadBo2SoundBank(const SoundAliasCsv::ZoneView& Z, int64_t Bank, const std::function<std::string(const std::string& Asset)>& File,
                      SoundAliasCsv::Rows& Out)
{
    auto& N = Out.Counters;
    SoundNames Names;
    Names.Add(KnownNames);
    Names.Add(Curves);
    // the bank's own: its aliases' and its ducks' (0x4c each: name char[32], id 0x20) names
    SoundAliasCsv::Rows Scratch;
    EachVariant(Z, Bank, 0x60, Scratch, [&](const std::string&, int64_t A) { Names.Add(Z.At<uint32_t>(A + 4), Z.Text(A)); });
    const int64_t Ducks = Z.Follow(Bank + 0x1c);
    for (uint32_t D = 0, Count = Z.At<uint32_t>(Bank + 0x18); Ducks >= 0 && D < Count; D++)
        Names.Add(Z.At<uint32_t>(Ducks + D * 0x4c + 0x20), Z.Chars(Ducks + D * 0x4c, 32));
    const auto Hashed = [&](uint32_t Hash) {
        bool Known = true;
        std::string S = Names.Of(Hash, Known);
        if (!Known) N["alias_hashes_unknown"]++;
        return S;
    };
    const auto Enum = [&](const std::string& S) {
        if (S.empty()) N["alias_enums_unknown"]++;
        return S;
    };

    // SndAlias (0x60): name 0, subtitle 8, secondary 0xc, asset file 0x14, flags 0x18/0x1c, duck 0x20,
    // context 0x24/0x28, stop on play 0x2c, futz 0x30, then u16s from 0x34 and bytes from 0x56
    EachVariant(Z, Bank, 0x60, Out, [&](const std::string& List, int64_t A) {
        SoundAliasCsv::Alias R;
        R[Col::Name] = Z.Text(A).empty() ? List : Z.Text(A);
        R[Col::Subtitle] = Z.Text(A + 8);
        R[Col::Secondary] = Z.Text(A + 0xc);
        R[Col::FileSpec] = File(Z.Text(A + 0x14));
        const uint32_t F0 = Z.At<uint32_t>(A + 0x18), F1 = Z.At<uint32_t>(A + 0x1c);
        R[Col::Looping] = Bits(F0, 0, 1) ? "looping" : "nonlooping";
        R[Col::PanType] = Bits(F0, 1, 1) ? "3d" : "2d";
        R[Col::DistanceLpf] = SoundAliasCsv::YesNo(Bits(F0, 2, 1));
        R[Col::Doppler] = SoundAliasCsv::YesNo(Bits(F0, 3, 1));
        R[Col::IsBig] = SoundAliasCsv::YesNo(Bits(F0, 4, 1));
        R[Col::Pauseable] = SoundAliasCsv::YesNo(Bits(F0, 5, 1));
        R[Col::IsMusic] = SoundAliasCsv::YesNo(Bits(F0, 6, 1));
        R[Col::StopOnEntDeath] = SoundAliasCsv::YesNo(Bits(F0, 7, 1));
        R[Col::Timescale] = SoundAliasCsv::YesNo(Bits(F0, 8, 1));
        R[Col::VoiceLimit] = SoundAliasCsv::YesNo(Bits(F0, 9, 1));
        R[Col::IgnoreMaxDist] = SoundAliasCsv::YesNo(Bits(F0, 10, 1));
        R[Col::Bus] = Enum(SoundAliasCsv::Named(Bits(F0, 11, 4), Buses));
        R[Col::Storage] = Enum(SoundAliasCsv::Named(Bits(F0, 15, 2), LoadTypes));
        R[Col::VolumeGroup] = Enum(SoundAliasCsv::Named(Bits(F0, 17, 5), Groups));
        R[Col::FluxType] = SoundAliasCsv::Named(Bits(F0, 22, 3), MoveTypes);
        R[Col::LimitType] = Enum(SoundAliasCsv::Named(Bits(F0, 25, 2), LimitTypes));
        R[Col::EntityLimitType] = Enum(SoundAliasCsv::Named(Bits(F0, 27, 2), LimitTypes));
        R[Col::RandomizeType] = Randomize(Bits(F0, 29, 3));
        R[Col::NeverPlayTwice] = SoundAliasCsv::YesNo(Bits(F1, 0, 1));
        R[Col::DryMaxCurve] = Enum(SoundAliasCsv::Named(Bits(F1, 2, 6), Curves));
        R[Col::WetMaxCurve] = Enum(SoundAliasCsv::Named(Bits(F1, 8, 6), Curves));
        R[Col::DryMinCurve] = Enum(SoundAliasCsv::Named(Bits(F1, 14, 6), Curves));
        R[Col::WetMinCurve] = Enum(SoundAliasCsv::Named(Bits(F1, 20, 6), Curves));
        R[Col::IsCinematic] = SoundAliasCsv::YesNo(Bits(F1, 27, 1));
        R[Col::Duck] = Hashed(Z.At<uint32_t>(A + 0x20));
        R[Col::ContextType] = Hashed(Z.At<uint32_t>(A + 0x24));
        R[Col::ContextValue] = Hashed(Z.At<uint32_t>(A + 0x28));
        R[Col::StopOnPlay] = Hashed(Z.At<uint32_t>(A + 0x2c));
        R[Col::FutzPatch] = Hashed(Z.At<uint32_t>(A + 0x30));
        R[Col::FluxTime] = std::to_string(Z.At<uint16_t>(A + 0x34));
        R[Col::StartDelay] = std::to_string(Z.At<uint16_t>(A + 0x36));
        R[Col::ReverbSend] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x38));
        R[Col::CenterSend] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x3a));
        R[Col::VolMin] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x3c));
        R[Col::VolMax] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x3e));
        R[Col::PitchMin] = SoundAliasCsv::CentsU16(Z.At<uint16_t>(A + 0x40), 32767.0);
        R[Col::PitchMax] = SoundAliasCsv::CentsU16(Z.At<uint16_t>(A + 0x42), 32767.0);
        R[Col::DistMin] = std::to_string(Z.At<uint16_t>(A + 0x44));
        R[Col::DistMaxDry] = std::to_string(Z.At<uint16_t>(A + 0x46));
        R[Col::DistMaxWet] = std::to_string(Z.At<uint16_t>(A + 0x48));
        R[Col::EnvelopMin] = std::to_string(Z.At<uint16_t>(A + 0x4a));
        R[Col::EnvelopMax] = std::to_string(Z.At<uint16_t>(A + 0x4c));
        R[Col::EnvelopPercent] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x4e));
        R[Col::FadeIn] = std::to_string(Z.At<int16_t>(A + 0x50));
        R[Col::FadeOut] = std::to_string(Z.At<int16_t>(A + 0x52));
        R[Col::DopplerScale] = std::to_string(Z.At<int16_t>(A + 0x54));
        R[Col::PriorityThresholdMin] = SoundAliasCsv::Fraction(Z.At<uint8_t>(A + 0x56));
        R[Col::PriorityThresholdMax] = SoundAliasCsv::Fraction(Z.At<uint8_t>(A + 0x57));
        R[Col::Probability] = SoundAliasCsv::Fraction(Z.At<uint8_t>(A + 0x58));
        R[Col::OcclusionLevel] = SoundAliasCsv::Fraction(Z.At<uint8_t>(A + 0x59));
        R[Col::PriorityMin] = std::to_string(Z.At<uint8_t>(A + 0x5a));
        R[Col::PriorityMax] = std::to_string(Z.At<uint8_t>(A + 0x5b));
        R[Col::Pan] = Enum(SoundAliasCsv::Named(Z.At<uint8_t>(A + 0x5c), Pans));
        R[Col::LimitCount] = std::to_string(Z.At<uint8_t>(A + 0x5d));
        R[Col::EntityLimitCount] = std::to_string(Z.At<uint8_t>(A + 0x5e));
        R[Col::DuckGroup] = Enum(SoundAliasCsv::Named(Z.At<uint8_t>(A + 0x5f), DuckGroups));
        Out.Aliases.push_back(std::move(R));
    });
}

void ReadBo1SoundBank(const SoundAliasCsv::ZoneView& Z, int64_t Bank, const std::function<std::string(int64_t SoundFile)>& File,
                      SoundAliasCsv::Rows& Out)
{
    auto& N = Out.Counters;
    SoundNames Names;
    Names.Add(KnownNames);
    SoundAliasCsv::Rows Scratch;
    EachVariant(Z, Bank, 0x54, Scratch, [&](const std::string&, int64_t A) { Names.Add(Z.At<uint32_t>(A + 4), Z.Text(A)); });
    const auto Hashed = [&](uint32_t Hash) {
        bool Known = true;
        std::string S = Names.Of(Hash, Known);
        if (!Known) N["alias_hashes_unknown"]++;
        return S;
    };
    // snd_alias_t (0x54): name 0, subtitle 8, secondary 0xc, SoundFile 0x10 (its type u8 at 4),
    // flags 0x14 (looping 0, 3D 1: BO2's first bits), duck 0x18, context 0x1c/0x20, u16s from 0x24,
    // bytes from 0x44
    EachVariant(Z, Bank, 0x54, Out, [&](const std::string& List, int64_t A) {
        SoundAliasCsv::Alias R;
        R[Col::Name] = Z.Text(A).empty() ? List : Z.Text(A);
        R[Col::Subtitle] = Z.Text(A + 8);
        R[Col::Secondary] = Z.Text(A + 0xc);
        const int64_t Spec = Z.Follow(A + 0x10);
        R[Col::FileSpec] = File(Spec);
        if (Spec >= 0) R[Col::Storage] = SoundAliasCsv::Named(Z.At<uint8_t>(Spec + 4), LoadTypes);
        const uint32_t F0 = Z.At<uint32_t>(A + 0x14);
        R[Col::Looping] = Bits(F0, 0, 1) ? "looping" : "nonlooping";
        R[Col::PanType] = Bits(F0, 1, 1) ? "3d" : "2d";
        R[Col::Duck] = Hashed(Z.At<uint32_t>(A + 0x18));
        R[Col::ContextType] = Hashed(Z.At<uint32_t>(A + 0x1c));
        R[Col::ContextValue] = Hashed(Z.At<uint32_t>(A + 0x20));
        R[Col::FluxTime] = std::to_string(Z.At<uint16_t>(A + 0x24));
        R[Col::StartDelay] = std::to_string(Z.At<uint16_t>(A + 0x26));
        R[Col::ReverbSend] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x28));
        R[Col::CenterSend] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x2a));
        R[Col::VolMin] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x2c));
        R[Col::VolMax] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x2e));
        R[Col::PitchMin] = SoundAliasCsv::CentsU16(Z.At<uint16_t>(A + 0x32), 32767.0);
        R[Col::PitchMax] = SoundAliasCsv::CentsU16(Z.At<uint16_t>(A + 0x34), 32767.0);
        R[Col::DistMin] = std::to_string(Z.At<uint16_t>(A + 0x38));
        R[Col::DistMaxDry] = std::to_string(Z.At<uint16_t>(A + 0x3a));
        R[Col::DistMaxWet] = std::to_string(Z.At<uint16_t>(A + 0x3c));
        R[Col::EnvelopMin] = std::to_string(Z.At<uint16_t>(A + 0x3e));
        R[Col::EnvelopMax] = std::to_string(Z.At<uint16_t>(A + 0x40));
        R[Col::EnvelopPercent] = SoundAliasCsv::Volume16(Z.At<uint16_t>(A + 0x42));
        R[Col::PriorityThresholdMin] = SoundAliasCsv::Fraction(Z.At<uint8_t>(A + 0x44));
        R[Col::PriorityThresholdMax] = SoundAliasCsv::Fraction(Z.At<uint8_t>(A + 0x45));
        R[Col::Probability] = SoundAliasCsv::Fraction(Z.At<uint8_t>(A + 0x46));
        R[Col::OcclusionLevel] = SoundAliasCsv::Fraction(Z.At<uint8_t>(A + 0x47));
        R[Col::PriorityMin] = std::to_string(Z.At<uint8_t>(A + 0x49));
        R[Col::PriorityMax] = std::to_string(Z.At<uint8_t>(A + 0x4a));
        // the curves (bytes 0x4c-0x4f) index a table of BO1's own: BO2's names fit a third of them
        R[Col::LimitCount] = std::to_string(Z.At<uint8_t>(A + 0x50));
        R[Col::EntityLimitCount] = std::to_string(Z.At<uint8_t>(A + 0x51));
        Out.Aliases.push_back(std::move(R));
    });
}
}
