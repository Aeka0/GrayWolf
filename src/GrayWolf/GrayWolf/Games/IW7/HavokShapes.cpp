#include "stdafx.h"

#include "Games/IW7/HavokShapes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <set>
#include <unordered_map>

namespace IW7Havok
{
namespace
{
    constexpr float Scale = 32.0f;   // game units per Havok unit

    template <class T>
    T Get(const uint8_t* Data, size_t Size, int64_t Off)
    {
        T V{};
        if (Off >= 0 && (size_t)Off + sizeof(T) <= Size) std::memcpy(&V, Data + Off, sizeof(T));
        return V;
    }

    // A binary packfile (hkPackfileHeader version 11, 64-bit little endian): the contents section's
    // objects (virtual fixups: offset, class name) and pointers (local fixups to the same section,
    // global ones to a section's offset); everything as offsets into the contents section
    class Pack
    {
    public:
        bool Open(const uint8_t* Data, size_t Size)
        {
            B = Data, N = Size;
            if (Get<uint32_t>(B, N, 0) != 0x57E0E057 || Get<uint32_t>(B, N, 4) != 0x10C0C010) return false;
            if (Get<int32_t>(B, N, 12) != 11 || Get<uint8_t>(B, N, 16) != 8 || Get<uint8_t>(B, N, 17) != 1) return false;
            const int32_t Sections = Get<int32_t>(B, N, 20), Contents = Get<int32_t>(B, N, 24);
            if (Sections <= 0 || Sections > 16 || Contents < 0 || Contents >= Sections) return false;
            // a section header (0x40 each from 0x40): its tag, then start, local fixups, global
            // fixups, virtual fixups, exports, imports, end
            std::vector<std::array<int32_t, 7>> Sec(Sections);
            for (int32_t K = 0; K < Sections; K++)
                for (int J = 0; J < 7; J++) Sec[K][J] = Get<int32_t>(B, N, 0x40 + K * 0x40 + 20 + J * 4);
            const auto& S = Sec[Contents];
            Base = S[0];
            if (Base < 0 || (size_t)Base + S[6] > N) return false;
            Length = S[1];
            for (int64_t O = S[1]; O + 8 <= S[2]; O += 8)
                if (const int32_t Src = Get<int32_t>(B, N, Base + O); Src != -1) Local[Src] = Get<int32_t>(B, N, Base + O + 4);
            for (int64_t O = S[2]; O + 12 <= S[3]; O += 12)
                if (const int32_t Src = Get<int32_t>(B, N, Base + O); Src != -1 && Get<int32_t>(B, N, Base + O + 4) == Contents)
                    Local[Src] = Get<int32_t>(B, N, Base + O + 8);
            for (int64_t O = S[3]; O + 12 <= S[4]; O += 12)
            {
                const int32_t Src = Get<int32_t>(B, N, Base + O), Section = Get<int32_t>(B, N, Base + O + 4);
                if (Src == -1 || Section < 0 || Section >= Sections) continue;
                const int64_t Name = (int64_t)Sec[Section][0] + Get<int32_t>(B, N, Base + O + 8);
                std::string Class;
                for (int64_t C = Name; C >= 0 && (size_t)C < N && B[C] && Class.size() < 128; C++) Class += (char)B[C];
                Classes[Src] = Class;
            }
            return true;
        }
        template <class T>
        T At(int64_t Off) const
        {
            return Off >= 0 && Off < Length ? Get<T>(B + Base, (size_t)Length, Off) : T{};
        }
        // where the pointer at Off points (-1: null or elsewhere)
        int64_t Ptr(int64_t Off) const
        {
            const auto It = Local.find((int32_t)Off);
            return It == Local.end() ? -1 : It->second;
        }
        // an hkArray at Off: its data and size
        std::pair<int64_t, int32_t> Array(int64_t Off) const
        {
            const int32_t Count = At<int32_t>(Off + 8);
            return { Count > 0 ? Ptr(Off) : -1, (std::max)(Count, 0) };
        }
        std::string ClassOf(int64_t Off) const
        {
            const auto It = Classes.find((int32_t)Off);
            return It == Classes.end() ? std::string() : It->second;
        }
        std::string String(int64_t Off) const
        {
            std::string S;
            for (int64_t C = Off; C >= 0 && C < Length && B[Base + C] && S.size() < 512; C++) S += (char)B[Base + C];
            return S;
        }
        bool Has(int64_t Off, int64_t Bytes) const { return Off >= 0 && Bytes >= 0 && Off + Bytes <= Length; }

    private:
        const uint8_t* B = nullptr;
        size_t N = 0;
        int64_t Base = 0, Length = 0;
        std::unordered_map<int32_t, int32_t> Local;
        std::unordered_map<int32_t, std::string> Classes;
    };

    // hknpShapeInstance's transform: rotation columns, translation, scale (in Havok's units)
    struct Transform
    {
        float Col[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
        float T[3] = { 0, 0, 0 }, S[3] = { 1, 1, 1 };
        bool Identity() const
        {
            for (int I = 0; I < 3; I++)
            {
                if (T[I] != 0 || S[I] != 1) return false;
                for (int J = 0; J < 3; J++)
                    if (Col[I][J] != (I == J ? 1.0f : 0.0f)) return false;
            }
            return true;
        }
        void Apply(const float* P, float* Out) const
        {
            for (int K = 0; K < 3; K++) Out[K] = Col[0][K] * P[0] * S[0] + Col[1][K] * P[1] * S[1] + Col[2][K] * P[2] * S[2] + T[K];
        }
    };

    class Reader
    {
    public:
        Reader(const Pack& P, std::map<std::string, size_t>& Counters) : P(P), Counters(Counters) {}

        std::vector<uint32_t> Contents;   // per shape tag

        void Read(int64_t Shape, uint16_t Tag, const Transform& X, IW7Havok::Shape& Out)
        {
            const std::string Class = P.ClassOf(Shape);
            if (Class == "hknpCompressedMeshShape")
                Mesh(P.Ptr(Shape + 0x60), X, Out);
            else if (Class == "hknpDynamicCompoundShape")
                Compound(Shape, X, Out);
            else if (Class == "hknpConvexPolytopeShape")
                Polytope(Shape, Tag, X, Out);
            else
                Counters["havok_shapes_unknown"]++;
        }

    private:
        const Pack& P;
        std::map<std::string, size_t>& Counters;

        uint32_t ContentsOf(uint32_t Tag) const { return Tag < Contents.size() ? Contents[Tag] : 0; }

        // hknpDynamicCompoundShape: its instances (0x80 each: rotation columns 0, 0x10, 0x20,
        // translation 0x30, scale 0x40, the shape 0x50, its tag u16 0x58), a free list array at 0x60
        // (none free on the maps measured)
        void Compound(int64_t Shape, const Transform& X, IW7Havok::Shape& Out)
        {
            if (!X.Identity())
            {
                Counters["havok_compounds_nested"]++;
                return;
            }
            const auto [Instances, Count] = P.Array(Shape + 0x60);
            for (int32_t K = 0; Instances >= 0 && K < Count; K++)
            {
                const int64_t I = Instances + (int64_t)K * 0x80;
                Transform Y;
                for (int C = 0; C < 3; C++)
                    for (int J = 0; J < 3; J++) Y.Col[C][J] = P.At<float>(I + C * 0x10 + J * 4);
                for (int J = 0; J < 3; J++) Y.T[J] = P.At<float>(I + 0x30 + J * 4), Y.S[J] = P.At<float>(I + 0x40 + J * 4);
                if (!Y.Identity()) Counters["havok_instances_moved"]++;
                Read(P.Ptr(I + 0x50), P.At<uint16_t>(I + 0x58), Y, Out);
            }
        }

        // hknpConvexPolytopeShape: vertices (an hkRelArray {u16 count, u16 offset from the field}
        // of vec4) at 0x30, planes (vec4: normal, d; n . p + d <= 0 inside) at 0x40
        void Polytope(int64_t Shape, uint16_t Tag, const Transform& X, IW7Havok::Shape& Out)
        {
            const uint16_t PlaneCount = P.At<uint16_t>(Shape + 0x40), PlaneOff = P.At<uint16_t>(Shape + 0x42);
            Maps::Brush B;
            B.Contents = ContentsOf(Tag);
            if (X.Identity())
            {
                for (uint16_t K = 0; K < PlaneCount; K++)
                {
                    const int64_t Pl = Shape + 0x40 + PlaneOff + K * 16;
                    Maps::BrushSide S;
                    for (int J = 0; J < 3; J++) S.Normal[J] = P.At<float>(Pl + J * 4);
                    S.Dist = -P.At<float>(Pl + 12) * Scale;
                    B.Sides.push_back(S);
                }
            }
            else
            {
                // moved: the hull of its moved vertices
                const uint16_t VertexCount = P.At<uint16_t>(Shape + 0x30), VertexOff = P.At<uint16_t>(Shape + 0x32);
                std::vector<float> Points;
                for (uint16_t K = 0; K < VertexCount; K++)
                {
                    float V[3], W[3];
                    for (int J = 0; J < 3; J++) V[J] = P.At<float>(Shape + 0x30 + VertexOff + K * 16 + J * 4);
                    X.Apply(V, W);
                    for (int J = 0; J < 3; J++) Points.push_back(W[J] * Scale);
                }
                if (!HullSides(Points, 0.1f, B.Sides)) B.Sides.clear();
            }
            if (B.Sides.size() < 4)
            {
                Counters["havok_polytopes_unread"]++;
                return;
            }
            Out.Brushes.push_back(std::move(B));
        }

        // hknpCompressedMeshShapeData: triangles, quads and convex pieces in sections of packed
        // (per section codec) and shared (tree domain) vertices
        void Mesh(int64_t Data, const Transform& X, IW7Havok::Shape& Out)
        {
            if (Data < 0)
            {
                Counters["havok_meshes_unread"]++;
                return;
            }
            float Min[3], Step[3];
            for (int K = 0; K < 3; K++)
            {
                Min[K] = P.At<float>(Data + 0x20 + K * 4);
                Step[K] = (P.At<float>(Data + 0x30 + K * 4) - Min[K]) / (float)((1u << (K < 2 ? 21 : 22)) - 1);
            }
            const auto [Sections, SectionCount] = P.Array(Data + 0x50);
            const auto [Prims, PrimCount] = P.Array(Data + 0x60);
            const auto [Numbers, NumberCount] = P.Array(Data + 0x70);
            const auto [Packed, PackedCount] = P.Array(Data + 0x80);
            const auto [Shared, SharedCount] = P.Array(Data + 0x90);
            const auto [Runs, RunCount] = P.Array(Data + 0xa0);
            if (!P.Has(Sections, (int64_t)SectionCount * 0x60) || !P.Has(Prims, (int64_t)PrimCount * 4) ||
                !P.Has(Numbers, (int64_t)NumberCount * 2) || !P.Has(Shared, (int64_t)SharedCount * 8))
            {
                Counters["havok_meshes_unread"]++;
                return;
            }
            const auto Place = [&](float* V) {
                float W[3];
                X.Apply(V, W);
                for (int K = 0; K < 3; K++) V[K] = W[K] * Scale;
            };
            // a shared vertex: 21, 21 and 22 bits across the tree's domain
            const auto SharedAt = [&](int64_t Number, float* V) {
                if (Number < 0 || Number >= SharedCount) return false;
                const uint64_t Q = P.At<uint64_t>(Shared + Number * 8);
                V[0] = Min[0] + (float)(Q & 0x1FFFFF) * Step[0];
                V[1] = Min[1] + (float)((Q >> 21) & 0x1FFFFF) * Step[1];
                V[2] = Min[2] + (float)(Q >> 42) * Step[2];
                Place(V);
                return true;
            };
            std::unordered_map<int64_t, uint32_t> Index;   // shared vertex (packed: ~its number) -> Out's
            for (int32_t S = 0; S < SectionCount; S++)
            {
                const int64_t Sec = Sections + (int64_t)S * 0x60;
                float Codec[6];   // packed vertices: offset, scale
                for (int K = 0; K < 6; K++) Codec[K] = P.At<float>(Sec + 0x30 + K * 4);
                const uint32_t FirstPacked = P.At<uint32_t>(Sec + 0x48), SharedOff = P.At<uint32_t>(Sec + 0x4c) >> 8;
                const uint32_t PrimWord = P.At<uint32_t>(Sec + 0x50), RunWord = P.At<uint32_t>(Sec + 0x54);
                const uint32_t PackedHere = P.At<uint8_t>(Sec + 0x58), SharedHere = P.At<uint8_t>(Sec + 0x59);
                const int64_t Page = (int64_t)P.At<uint8_t>(Sec + 0x5c) << 16;   // the shared numbers' high bits
                // a data run: {u16 tag, u8 first primitive, u8 count}
                std::array<uint16_t, 256> Tags;
                Tags.fill(0xffff);
                for (uint32_t R = 0; R < (RunWord & 0xff) && (RunWord >> 8) + R < (uint32_t)RunCount; R++)
                {
                    const int64_t Run = Runs + (int64_t)((RunWord >> 8) + R) * 4;
                    const uint8_t First = P.At<uint8_t>(Run + 2), Count = P.At<uint8_t>(Run + 3);
                    for (uint32_t K = First; K < (uint32_t)First + Count && K < 256; K++) Tags[K] = P.At<uint16_t>(Run);
                }
                const auto SharedNumber = [&](uint32_t J) -> int64_t {
                    if (J >= SharedHere || SharedOff + J >= (uint32_t)NumberCount) return -1;
                    return Page | P.At<uint16_t>(Numbers + (int64_t)(SharedOff + J) * 2);
                };
                // a primitive's vertex -> Out's, or -1; packed ones 11, 11 and 10 bits in the codec
                const auto Vertex = [&](uint32_t I) -> int64_t {
                    int64_t Key;
                    float V[3];
                    if (I < PackedHere)
                    {
                        const int64_t Number = (int64_t)FirstPacked + I;
                        if (Number >= PackedCount) return -1;
                        Key = ~Number;
                        if (const auto It = Index.find(Key); It != Index.end()) return It->second;
                        const uint32_t Q = P.At<uint32_t>(Packed + Number * 4);
                        V[0] = Codec[0] + (float)(Q & 0x7FF) * Codec[3];
                        V[1] = Codec[1] + (float)((Q >> 11) & 0x7FF) * Codec[4];
                        V[2] = Codec[2] + (float)(Q >> 22) * Codec[5];
                        Place(V);
                    }
                    else
                    {
                        Key = SharedNumber(I - PackedHere);
                        if (Key < 0) return -1;
                        if (const auto It = Index.find(Key); It != Index.end()) return It->second;
                        if (!SharedAt(Key, V)) return -1;
                    }
                    const uint32_t Number = (uint32_t)(Out.Vertices.size() / 3);
                    Out.Vertices.insert(Out.Vertices.end(), V, V + 3);
                    Index.emplace(Key, Number);
                    return Number;
                };
                for (uint32_t K = 0; K < (PrimWord & 0xff) && (PrimWord >> 8) + K < (uint32_t)PrimCount; K++)
                {
                    const int64_t Prim = Prims + (int64_t)((PrimWord >> 8) + K) * 4;
                    const uint8_t A = P.At<uint8_t>(Prim), Bv = P.At<uint8_t>(Prim + 1), C = P.At<uint8_t>(Prim + 2), D = P.At<uint8_t>(Prim + 3);
                    const uint32_t Contents = ContentsOf(Tags[K]);
                    if (Bv == C && C == D)
                    {
                        // a convex piece: at a, its vertex count << 8 | flags, then its first shared vertex
                        const int64_t Head = A >= PackedHere ? SharedNumber(A - PackedHere) : -1;
                        const int64_t First = A >= PackedHere ? SharedNumber(A + 1 - PackedHere) : -1;
                        std::vector<float> Points;
                        float V[3];
                        for (int64_t M = 0; Head >= 0 && First >= 0 && M < ((Head & 0xffff) >> 8) && SharedAt(First + M, V); M++)
                            Points.insert(Points.end(), V, V + 3);
                        Maps::Brush B;
                        B.Contents = Contents;
                        if (!HullSides(Points, 0.1f, B.Sides))
                        {
                            Counters["havok_convex_unread"]++;
                            continue;
                        }
                        Out.Brushes.push_back(std::move(B));
                        continue;
                    }
                    // a triangle (a, b, c, c) or a quad (a, b, c and a, c, d)
                    const int64_t Ia = Vertex(A), Ib = Vertex(Bv), Ic = Vertex(C), Id = C != D ? Vertex(D) : 0;
                    if (Ia < 0 || Ib < 0 || Ic < 0 || Id < 0)
                    {
                        Counters["havok_triangles_unread"]++;
                        continue;
                    }
                    Out.Triangles.insert(Out.Triangles.end(), { (uint32_t)Ia, (uint32_t)Ib, (uint32_t)Ic });
                    Out.TriangleContents.push_back(Contents);
                    if (C != D)
                    {
                        Out.Triangles.insert(Out.Triangles.end(), { (uint32_t)Ia, (uint32_t)Ic, (uint32_t)Id });
                        Out.TriangleContents.push_back(Contents);
                    }
                }
            }
        }
    };
}

bool HullSides(const std::vector<float>& Points, float Eps, std::vector<Maps::BrushSide>& Out)
{
    using V3 = std::array<double, 3>;
    const size_t N = Points.size() / 3;
    if (N < 4 || N > 255) return false;
    std::vector<V3> Pt(N);
    for (size_t K = 0; K < N; K++) Pt[K] = { Points[K * 3], Points[K * 3 + 1], Points[K * 3 + 2] };
    const auto Sub = [](const V3& A, const V3& B) { return V3{ A[0] - B[0], A[1] - B[1], A[2] - B[2] }; };
    const auto Dot = [](const V3& A, const V3& B) { return A[0] * B[0] + A[1] * B[1] + A[2] * B[2]; };
    const auto Cross = [](const V3& A, const V3& B) {
        return V3{ A[1] * B[2] - A[2] * B[1], A[2] * B[0] - A[0] * B[2], A[0] * B[1] - A[1] * B[0] };
    };
    std::set<std::vector<bool>> Faces;   // the points of each face found
    for (size_t I = 0; I < N; I++)
        for (size_t J = I + 1; J < N; J++)
            for (size_t K = J + 1; K < N; K++)
            {
                V3 Nm = Cross(Sub(Pt[J], Pt[I]), Sub(Pt[K], Pt[I]));
                const double L = std::sqrt(Dot(Nm, Nm));
                if (L < 1e-6) continue;
                for (double& C : Nm) C /= L;
                const double D = Dot(Nm, Pt[I]);
                double Lo = 0, Hi = 0;
                for (const V3& Q : Pt) Lo = (std::min)(Lo, Dot(Nm, Q) - D), Hi = (std::max)(Hi, Dot(Nm, Q) - D);
                if (Hi > Eps && Lo < -Eps) continue;   // points on both sides: no face
                if (Hi > Eps)
                    for (double& C : Nm) C = -C;
                std::vector<bool> On(N);
                for (size_t M = 0; M < N; M++) On[M] = std::fabs(Dot(Nm, Pt[M]) - Dot(Nm, Pt[I])) <= Eps;
                if (!Faces.insert(On).second) continue;
                // the face's plane from its widest three points, an axis' when within 1/1000 of it
                double Best = L;
                for (size_t A = 0; A < N; A++)
                    for (size_t B = A + 1; On[A] && B < N; B++)
                        for (size_t C = B + 1; On[B] && C < N; C++)
                        {
                            if (!On[C]) continue;
                            V3 M2 = Cross(Sub(Pt[B], Pt[A]), Sub(Pt[C], Pt[A]));
                            const double L2 = std::sqrt(Dot(M2, M2));
                            if (L2 <= Best) continue;
                            const double Sign = Dot(M2, Nm) < 0 ? -1.0 : 1.0;
                            for (double& X : M2) X *= Sign / L2;
                            Nm = M2, Best = L2;
                        }
                for (int A = 0; A < 3; A++)
                    if (std::fabs(Nm[A]) > 0.999)
                    {
                        const double Sign = Nm[A] < 0 ? -1.0 : 1.0;
                        Nm = { 0, 0, 0 };
                        Nm[A] = Sign;
                    }
                double Far = -1e30;   // every point inside
                for (const V3& Q : Pt) Far = (std::max)(Far, Dot(Nm, Q));
                bool Same = false;
                for (const Maps::BrushSide& S : Out)
                    Same = Same || (S.Normal[0] * Nm[0] + S.Normal[1] * Nm[1] + S.Normal[2] * Nm[2] > 0.99999 && std::fabs(S.Dist - Far) < Eps);
                if (Same) continue;
                Maps::BrushSide S;
                for (int A = 0; A < 3; A++) S.Normal[A] = (float)Nm[A];
                S.Dist = (float)Far;
                Out.push_back(S);
            }
    return Out.size() >= 4;
}

bool ReadShapeList(const uint8_t* Data, size_t Size, std::vector<Shape>& Out, std::map<std::string, size_t>& Counters)
{
    // HavokPhysicsShapeList, the contents' first object: shapes 0 (pointers), names 0x20
    // (pointers), the shape tags' records 0x68 (24 bytes, content flags at 0)
    Pack P;
    if (!P.Open(Data, Size) || P.ClassOf(0) != "HavokPhysicsShapeList")
    {
        Counters["havok_packfiles_unread"]++;
        return false;
    }
    Reader R(P, Counters);
    const auto [Tags, TagCount] = P.Array(0x68);
    for (int32_t K = 0; Tags >= 0 && K < TagCount; K++) R.Contents.push_back(P.At<uint32_t>(Tags + (int64_t)K * 24));
    const auto [Shapes, Count] = P.Array(0);
    const auto [Names, NameCount] = P.Array(0x20);
    Out.assign((size_t)Count, {});
    for (int32_t K = 0; Shapes >= 0 && K < Count; K++)
    {
        if (K < NameCount && Names >= 0) Out[K].Name = P.String(P.Ptr(Names + (int64_t)K * 8));
        const int64_t S = P.Ptr(Shapes + (int64_t)K * 8);
        if (S < 0)
        {
            Counters["havok_shapes_unread"]++;
            continue;
        }
        R.Read(S, 0xffff, {}, Out[K]);
    }
    return true;
}
}
