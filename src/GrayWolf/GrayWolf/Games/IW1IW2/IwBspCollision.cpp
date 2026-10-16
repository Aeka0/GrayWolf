#include "stdafx.h"

#include "ExportNaming.h"
#include "IwBsp.h"
#include "MapRadiantWriter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <unordered_map>

// The collision of a CoD1 / CoD2 level for the map writers (Maps::Collision).

namespace IW12
{
namespace
{
    using V3 = std::array<double, 3>;

    // Content flags (Quake 3's lineage, as CoD1's common.shader surfaceparms set them and CoD4 on
    // keep them): what a surface stops. A collision surface whose material stops none of these
    // (transparent, foliage, water, nosight only) is no collision triangle.
    constexpr uint32_t Solid = 0x1, CanShootClip = 0x40, MissileClip = 0x80, VehicleClip = 0x200, ItemClip = 0x400,
                       ClipShot = 0x2000, PlayerClip = 0x10000, MonsterClip = 0x20000;
    constexpr uint32_t Stops = Solid | CanShootClip | MissileClip | VehicleClip | ItemClip | ClipShot | PlayerClip | MonsterClip;

    // CoD1's tool shaders (textures/common/<x>) under the names CoD2 on gives the same tools, so
    // the Radiant writer recognises them; a shader not listed keeps its name (its brush then
    // takes the tool its content flags give)
    std::string ToolName(const std::string& Name)
    {
        static const std::unordered_map<std::string, std::string> Renamed = {
            { "clipfull", "clip_full" },       { "clipplayer", "clip_player" },   { "clipmonster", "clip_ai" },
            { "clipmissile", "clip_missile" }, { "clipfoliage", "clip_foliage" }, { "vehicleclip", "clip_vehicle" },
            { "water_clip", "clip_water" },    { "portalnodraw", "portal_nodraw" }, { "aitrig", "trigger" },
        };
        static const char* const Same[] = { "caulk", "clip", "clip_metal", "clip_nosight", "clip_nosight_dirt", "clip_nosight_metal",
                                            "clip_nosight_nothing", "clip_nosight_rock", "hint", "ladder", "nodraw", "nodraw_notsolid",
                                            "nodraw_decal", "nosight_noclip", "portal", "trigger" };
        const std::string Lower = ExportNaming::Lower(Name);
        constexpr const char Common[] = "textures/common/";
        if (Lower.rfind(Common, 0) != 0) return Name;
        const std::string Tool = Lower.substr(sizeof(Common) - 1);
        if (const auto It = Renamed.find(Tool); It != Renamed.end()) return It->second;
        for (const char* S : Same)
            if (Tool == S) return Tool;
        return Name;
    }

    // The triangles collected: vertices once each (by their number in the source) in Out, three
    // numbers each into Target, and with Materials the material of each
    struct Triangles
    {
        Maps::Collision& Out;
        std::vector<uint32_t>* Target;
        std::vector<uint32_t>* Materials = nullptr;
        uint32_t Material = 0;
        std::unordered_map<uint64_t, uint32_t> Index;
        uint32_t Add(uint64_t Key, const V3& P)
        {
            const auto [It, New] = Index.emplace(Key, (uint32_t)(Out.Vertices.size() / 3));
            if (New)
                for (const double C : P) Out.Vertices.push_back((float)C);
            return It->second;
        }
        // a triangle facing out where (B - A) x (C - A) points; false for one without area
        bool Push(uint32_t A, uint32_t B, uint32_t C)
        {
            const auto At = [this](uint32_t I) { return V3{ Out.Vertices[3 * I], Out.Vertices[3 * I + 1], Out.Vertices[3 * I + 2] }; };
            const V3 Pa = At(A), Pb = At(B), Pc = At(C);
            const V3 U = { Pb[0] - Pa[0], Pb[1] - Pa[1], Pb[2] - Pa[2] }, W = { Pc[0] - Pa[0], Pc[1] - Pa[1], Pc[2] - Pa[2] };
            const V3 N = { U[1] * W[2] - U[2] * W[1], U[2] * W[0] - U[0] * W[2], U[0] * W[1] - U[1] * W[0] };
            if (N[0] * N[0] + N[1] * N[1] + N[2] * N[2] < 1e-12) return false;
            Target->insert(Target->end(), { A, B, C });
            if (Materials) Materials->push_back(Material);
            return true;
        }
    };

    // The steps a quadratic Bezier span is cut into: its chords then stay within Tolerance of it
    // (a span's curve leaves its N-chord polyline by at most |P0 - 2 P1 + P2| / (4 N^2))
    int Steps(const float* P0, const float* P1, const float* P2)
    {
        constexpr double Tolerance = 1.0;
        double D = 0;
        for (int K = 0; K < 3; K++) D += std::pow((double)P0[K] - 2.0 * P1[K] + P2[K], 2);
        const double Deviation = std::sqrt(D) / 4.0;
        return (std::min)(16, (std::max)(1, (int)std::ceil(std::sqrt(Deviation / Tolerance) - 1e-9)));
    }

    // CoD1: a patch (a grid of Width x Height Bezier control points, odd counts, 3 x 3 per span)
    // tessellated, each span cut into as many steps as its row (column) needing the most; the grid
    // faces out along (next column - column) x (next row - row), as the drawn normals say
    void Tessellate(const BspFile& Bsp, const BspFile::Patch& S, uint32_t Number, Triangles& Into, size_t& Dropped)
    {
        const float* Cp = &Bsp.CollisionVertices[3 * (size_t)S.FirstVertex];
        const auto Ctl = [&](int Row, int Col) { return Cp + 3 * ((size_t)Row * S.Width + Col); };
        // the parameter of each tessellated column (span, t) and row
        const auto Params = [&](int Spans, bool Columns) {
            std::vector<std::pair<int, double>> Out;
            for (int Sp = 0; Sp < Spans; Sp++)
            {
                int N = 1;
                const int Lines = Columns ? S.Height : S.Width;
                for (int L = 0; L < Lines; L++)
                    N = (std::max)(N, Columns ? Steps(Ctl(L, 2 * Sp), Ctl(L, 2 * Sp + 1), Ctl(L, 2 * Sp + 2))
                                              : Steps(Ctl(2 * Sp, L), Ctl(2 * Sp + 1, L), Ctl(2 * Sp + 2, L)));
                for (int K = 0; K < N; K++) Out.emplace_back(Sp, (double)K / N);
            }
            Out.emplace_back(Spans - 1, 1.0);
            return Out;
        };
        const auto Us = Params(S.Width / 2, true), Vs = Params(S.Height / 2, false);
        std::vector<uint32_t> Grid(Us.size() * Vs.size());
        for (size_t R = 0; R < Vs.size(); R++)
            for (size_t C = 0; C < Us.size(); C++)
            {
                const auto [Su, U] = Us[C];
                const auto [Sv, V] = Vs[R];
                const double Bu[3] = { (1 - U) * (1 - U), 2 * U * (1 - U), U * U }, Bv[3] = { (1 - V) * (1 - V), 2 * V * (1 - V), V * V };
                V3 P = { 0, 0, 0 };
                for (int J = 0; J < 3; J++)
                    for (int I = 0; I < 3; I++)
                    {
                        const float* Q = Ctl(2 * Sv + J, 2 * Su + I);
                        for (int K = 0; K < 3; K++) P[K] += Bu[I] * Bv[J] * Q[K];
                    }
                // keyed by the patch and the grid point
                Grid[R * Us.size() + C] = Into.Add((1ull << 63) | ((uint64_t)Number << 32) | (R * Us.size() + C), P);
            }
        for (size_t R = 0; R + 1 < Vs.size(); R++)
            for (size_t C = 0; C + 1 < Us.size(); C++)
            {
                const uint32_t A = Grid[R * Us.size() + C], B = Grid[R * Us.size() + C + 1], D = Grid[(R + 1) * Us.size() + C],
                               E = Grid[(R + 1) * Us.size() + C + 1];
                if (!Into.Push(A, B, D)) Dropped++;
                if (!Into.Push(B, E, D)) Dropped++;
            }
    }

    // CoD2: a triangle's vertex where its plane and barycentric planes put it (s, t)
    V3 Barycentric(const BspFile::Triangle& T, double S, double Tt)
    {
        const double M[3][3] = { { T.Plane[0], T.Plane[1], T.Plane[2] }, { T.S[0], T.S[1], T.S[2] }, { T.T[0], T.T[1], T.T[2] } };
        const double R[3] = { T.Plane[3], T.S[3] + S, T.T[3] + Tt };
        const auto Det = [](const double A[3][3]) {
            return A[0][0] * (A[1][1] * A[2][2] - A[1][2] * A[2][1]) - A[0][1] * (A[1][0] * A[2][2] - A[1][2] * A[2][0]) +
                   A[0][2] * (A[1][0] * A[2][1] - A[1][1] * A[2][0]);
        };
        const double D = Det(M);
        V3 Out = { 0, 0, 0 };
        if (std::fabs(D) < 1e-20) return Out;
        for (int C = 0; C < 3; C++)
        {
            double A[3][3];
            std::memcpy(A, M, sizeof(A));
            for (int K = 0; K < 3; K++) A[K][C] = R[K];
            Out[C] = Det(A) / D;
        }
        return Out;
    }
}

void BspCollision(const BspFile& Bsp, Maps::Collision& Out, std::map<std::string, size_t>& Counters)
{
    Out = Maps::Collision();
    std::vector<std::string> Names(Bsp.Materials.size());
    for (size_t I = 0; I < Bsp.Materials.size(); I++)
    {
        Names[I] = ToolName(Bsp.Materials[I].Name);
        Out.Materials.emplace_back(Names[I], Bsp.Materials[I].ContentFlags);
    }

    // brushes: the first six sides the bounds (min x, max x, ...), the others planes
    std::vector<size_t> FirstSide(Bsp.Brushes.size());
    for (size_t I = 0, Side = 0; I < Bsp.Brushes.size(); Side += Bsp.Brushes[I].Sides, I++) FirstSide[I] = Side;
    const auto BrushOf = [&](size_t I) {
        const auto& B = Bsp.Brushes[I];
        Maps::Brush Brush;
        Brush.Contents = Bsp.Materials[B.Material].ContentFlags;
        for (size_t K = 0; K < B.Sides; K++)
        {
            const auto& S = Bsp.BrushSides[FirstSide[I] + K];
            Maps::BrushSide Side;
            if (K < 6)
            {
                float Bound;
                std::memcpy(&Bound, &S.PlaneOrBound, 4);
                const float Sign = K % 2 ? 1.0f : -1.0f;
                Side.Normal[0] = Side.Normal[1] = Side.Normal[2] = 0;
                Side.Normal[K / 2] = Sign;
                Side.Dist = Sign * Bound;
            }
            else
            {
                const auto& P = Bsp.Planes[S.PlaneOrBound];
                std::memcpy(Side.Normal, P.Normal, 12);
                Side.Dist = P.Dist;
            }
            Side.Material = Names[S.Material];
            Side.Contents = Bsp.Materials[S.Material].ContentFlags;
            Side.Surface = Bsp.Materials[S.Material].SurfaceFlags;
            Brush.Sides.push_back(std::move(Side));
        }
        return Brush;
    };
    Out.ClipBrushes = Bsp.Brushes.size();
    Out.Models.resize(Bsp.Models.size());
    for (size_t M = 0; M < Bsp.Models.size(); M++)
    {
        auto& Into = M ? Out.Models[M] : Out.Brushes;
        for (uint32_t B = 0; B < Bsp.Models[M].Brushes; B++) Into.push_back(BrushOf(Bsp.Models[M].FirstBrush + B));
    }

    // the curved and terrain collision as triangles: the world's in Out, each brush model's
    // gathered apart (in its own space, like its brushes), then made brushes of the model
    Triangles Tri{ Out, &Out.Triangles };
    Maps::Collision Gathered;
    std::vector<std::vector<uint32_t>> ModelTris(Bsp.Models.size()), ModelMats(Bsp.Models.size());
    Triangles ModelTri{ Gathered, nullptr };
    // the collector for model M's triangles of material Material
    const auto Into = [&](size_t M, uint32_t Material) -> Triangles& {
        if (!M) return Tri;
        ModelTri.Target = &ModelTris[M];
        ModelTri.Materials = &ModelMats[M];
        ModelTri.Material = Material;
        return ModelTri;
    };
    size_t Degenerate = 0;
    const auto Collides = [&](size_t Material) { return (Bsp.Materials[Material].ContentFlags & Stops) != 0; };
    if (!Bsp.Patches.empty())
    {
        std::vector<char> Done(Bsp.Patches.size(), 0);
        for (size_t M = 0; M < Bsp.Models.size(); M++)
            for (uint32_t K = 0; K < Bsp.Models[M].Collisions; K++)
            {
                const uint32_t N = Bsp.PatchRefs[Bsp.Models[M].FirstCollision + K];
                if (Done[N]) continue;
                Done[N] = 1;
                const auto& S = Bsp.Patches[N];
                Triangles& To = Into(M, S.Material);
                if (!Collides(S.Material))
                {
                    Counters["collision_surfaces_not_colliding"]++;
                    continue;
                }
                Counters[S.Type == 1 ? (M ? "collision_model_terrain_surfaces" : "collision_terrain_surfaces")
                                     : (M ? "collision_model_patches" : "collision_patches")]++;
                if (S.Type == 0)
                {
                    Tessellate(Bsp, S, (uint32_t)N, To, Degenerate);
                    continue;
                }
                // terrain: indices from its first vertex, stored clockwise
                for (uint32_t I = 0; I + 2 < S.Height; I += 3)
                {
                    uint32_t V[3];
                    for (int J = 0; J < 3; J++)
                    {
                        const size_t Vi = (size_t)S.Other + Bsp.CollisionIndices[S.FirstVertex + I + J];
                        const float* P = &Bsp.CollisionVertices[3 * Vi];
                        V[J] = To.Add(Vi, { P[0], P[1], P[2] });
                    }
                    if (!To.Push(V[0], V[2], V[1])) Degenerate++;
                }
            }
        for (size_t N = 0; N < Done.size(); N++)
            if (!Done[N]) Counters["collision_surfaces_unreferenced"]++;
    }
    if (!Bsp.Triangles.empty())
    {
        // the brush model owning each tree node (0: the world's): the nodes under its roots
        std::vector<uint32_t> Owner(Bsp.AabbTree.size(), 0);
        for (size_t M = 1; M < Bsp.Models.size(); M++)
        {
            std::vector<uint32_t> Stack;
            for (uint32_t K = 0; K < Bsp.Models[M].Collisions; K++) Stack.push_back(Bsp.Models[M].FirstCollision + K);
            while (!Stack.empty())
            {
                const uint32_t N = Stack.back();
                Stack.pop_back();
                if (Owner[N]) continue;
                Owner[N] = (uint32_t)M;
                for (uint32_t C = 0; C < Bsp.AabbTree[N].Children; C++) Stack.push_back(Bsp.AabbTree[N].First + C);
            }
        }
        // per triangle: 0 not reached, 1 colliding, 2 not colliding (a leaf whose material
        // collides wins, the world's over a model's); and whose it is
        std::vector<char> Use(Bsp.Triangles.size(), 0);
        std::vector<uint32_t> Whose(Bsp.Triangles.size(), 0), Material(Bsp.Triangles.size(), 0);
        for (size_t N = 0; N < Bsp.AabbTree.size(); N++)
        {
            const auto& A = Bsp.AabbTree[N];
            if (A.Children) continue;
            const char U = Collides(A.Material) ? 1 : 2;
            const auto& P = Bsp.Partitions[A.First];
            for (uint32_t K = 0; K < P.Triangles; K++)
            {
                const size_t I = P.FirstTriangle + K;
                if (Use[I] == 0 || (U == 1 && (Use[I] == 2 || (Owner[N] == 0 && Whose[I] != 0))))
                    Use[I] = U, Whose[I] = Owner[N], Material[I] = A.Material;
            }
        }
        for (size_t I = 0; I < Bsp.Triangles.size(); I++)
        {
            if (Use[I] != 1)
            {
                Counters[Use[I] == 0 ? "collision_triangles_unreached" : "collision_triangles_not_colliding"]++;
                continue;
            }
            Triangles& To = Into(Whose[I], Material[I]);
            const auto& T = Bsp.Triangles[I];
            uint32_t V[3];
            for (int J = 0; J < 3; J++)
            {
                const uint32_t Vi = T.Vertices[J];
                if (Vi == 0xFFFFFFFF)
                {
                    // where s and t put it (0, 0), (1, 0), (0, 1): kept apart per triangle
                    V[J] = To.Add((1ull << 63) | ((uint64_t)I << 2) | (uint64_t)J, Barycentric(T, J == 1, J == 2));
                    Counters["collision_vertices_rebuilt"]++;
                }
                else
                {
                    const float* P = &Bsp.CollisionVertices[3 * (size_t)Vi];
                    V[J] = To.Add(Vi, { P[0], P[1], P[2] });
                }
            }
            // facing out along the triangle's plane
            const auto& Vx = To.Out.Vertices;
            const auto At = [&](uint32_t K) { return V3{ Vx[3 * K], Vx[3 * K + 1], Vx[3 * K + 2] }; };
            const V3 A = At(V[0]), B = At(V[1]), C = At(V[2]);
            const V3 U = { B[0] - A[0], B[1] - A[1], B[2] - A[2] }, W = { C[0] - A[0], C[1] - A[1], C[2] - A[2] };
            const double Along = (U[1] * W[2] - U[2] * W[1]) * T.Plane[0] + (U[2] * W[0] - U[0] * W[2]) * T.Plane[1] +
                                 (U[0] * W[1] - U[1] * W[0]) * T.Plane[2];
            if (!(Along >= 0 ? To.Push(V[0], V[1], V[2]) : To.Push(V[0], V[2], V[1]))) Degenerate++;
        }
    }
    if (Degenerate) Counters["collision_triangles_degenerate"] += Degenerate;

    // a brush model's triangles as brushes of it: each the prism the Radiant export makes of a
    // world triangle, with its material
    constexpr double PrismDepth = 2.0;
    for (size_t M = 1; M < Bsp.Models.size(); M++)
        for (size_t T = 0; T < ModelMats[M].size(); T++)
        {
            std::array<double, 3> P[3];
            for (int J = 0; J < 3; J++)
                for (int A = 0; A < 3; A++) P[J][A] = Gathered.Vertices[3 * (size_t)ModelTris[M][3 * T + J] + A];
            Maps::Brush Br;
            if (!Maps::TrianglePrism(P, PrismDepth, Br))
            {
                Counters["collision_model_triangles_dropped"]++;
                continue;
            }
            const uint32_t Mat = ModelMats[M][T];
            Br.Contents = Bsp.Materials[Mat].ContentFlags;
            for (auto& S : Br.Sides)
            {
                S.Material = Names[Mat];
                S.Contents = Bsp.Materials[Mat].ContentFlags;
                S.Surface = Bsp.Materials[Mat].SurfaceFlags;
            }
            Out.Models[M].push_back(std::move(Br));
            Counters["collision_model_triangles"]++;
        }
}
}
