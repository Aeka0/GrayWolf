#include "stdafx.h"

// BO3 vehicle (56) -> its GDT entry: the fields are the type's data (zonekit/profiles/t7/gdt/vehicle);
// what a table cannot say is here, as HydraX's Vehicle.cs does (github.com/Scobalula/HydraX,
// GPL-3.0): the mantle angles the game keeps as cosines (written acos(x) in degrees * 2), and the
// two enums whose numbers their tables do not name (a specialty past the list: "none"; a team
// other than allies/axis/team3/neutral: "").

#include "Gdt.h"

#include <cmath>
#include <cstdlib>

namespace
{
    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out,
                 Gdt::Stats& S)
    {
        Gdt::Block B{ Name, "vehicle.gdf", "", T.Fields(M, Pos, S) };
        const auto Numeric = [](const std::string& V) {
            if (V.empty()) return false;
            char* End = nullptr;
            std::strtod(V.c_str(), &End);
            return End && *End == 0;
        };
        for (auto& [K, V] : B.Keys)
            if (K.rfind("mantleAngle", 0) == 0)
            {
                const double X = std::acos((double)std::strtof(V.c_str(), nullptr)) / 0.017453292 / 0.5;
                V = Gdt::Number(X, false);
            }
            else if (K == "targetingImmuneSpecialty" && Numeric(V)) V = "none";
            else if (K == "team" && Numeric(V)) V.clear();
        Out.push_back(std::move(B));
        return true;
    }
}

GDT_CONVERTER(t7_vehicle, Convert);
