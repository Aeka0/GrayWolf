#include "stdafx.h"

// BO3 weapon (23) -> its GDT entry: the fields are the type's data (zonekit/profiles/t7/gdt/weapon);
// the GDF is chosen here, as HydraX's Weapon.cs does (github.com/Scobalula/HydraX, GPL-3.0): the
// game keeps no GDF, the Linker only looks it up in the gdtdb.

#include "Gdt.h"

#include <cstdlib>

namespace
{
    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out,
                 Gdt::Stats& S)
    {
        Gdt::Block B{ Name, "", "", T.Fields(M, Pos, S) };
        const auto Get = [&](const char* Key) {
            for (const auto& [K, V] : B.Keys)
                if (K == Key) return V;
            return std::string();
        };
        const std::string Type = Get("weaponType");
        const bool Projectile = Type == "projectile";
        std::string Gdf;
        if (Get("dualWield") == "1")
            Gdf = Get("viewmodelTag") == "tag_weapon_right" ? (Projectile ? "projectile" : "bullet")
                                                             : (Projectile ? "dualwieldprojectile" : "dualwieldbullet");
        else if (std::atof(Get("minHorTurnSpeed").c_str()) >= 1.0) Gdf = "turret";
        else if (std::atof(Get("maxDist").c_str()) >= 1.0)
            Gdf = Projectile || Type == "bomb" ? "projectile" : Type == "gas" ? "gas" : "bullet";
        else Gdf = Type;
        B.Gdf = Gdf + "weapon.gdf";
        S.Counters["weapon_" + Gdf]++;
        Out.push_back(std::move(B));
        return true;
    }
}

GDT_CONVERTER(t7_weapon, Convert);
