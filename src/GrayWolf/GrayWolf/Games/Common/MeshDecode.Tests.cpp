#include "stdafx.h"

// MeshDecode's checks (run by `self-test`): each vertex codec on a vertex taken from a real
// surface (its bytes and what the game's reader made of them, bit for bit), the face codecs and
// the weight layouts on small hand-built buffers, and every read that would leave the buffer.

#include "Games/Common/MeshDecode.h"

#include "WraithModel.h"

#include <bit>
#include <cstring>
#include <format>
#include <vector>

namespace
{
    std::vector<uint8_t> FromHex(const char* Hex)
    {
        std::vector<uint8_t> Out;
        for (size_t I = 0; Hex[I] && Hex[I + 1]; I += 2) Out.push_back((uint8_t)std::stoul(std::string(Hex + I, 2), nullptr, 16));
        return Out;
    }

    // One vertex as a game's buffer holds it: positions, normal and UV bytes (hex), the surface's
    // scale and offset, and the position, normal and UV the reader made of them (float bits)
    struct RealVertex
    {
        const char* What;
        MeshDecode::PositionCodec Position;
        MeshDecode::NormalCodec Normal;
        const char *Pos, *Nrm, *Uv;
        uint32_t Scale, Offset[3];
        uint32_t P[3], N[3], U[2];
    };
    // REAL-VECTORS
    const std::vector<RealVertex> Vertices = {
        { "Modern Warfare III (21-bit position, QTangent)", MeshDecode::PositionCodec::Packed21, MeshDecode::NormalCodec::QTangent,
          "23bfaf740a566d42", "e887f56d", "d738c33b", 0x43462715, { 0, 0, 0 },
          { 0xc048d1c8, 0x40817d8c, 0x40f077b0 }, { 0xbed7da29, 0xbe3d08e1, 0xbf6346c3 }, { 0x3f1ae000, 0x3f786000 } },
        { "Black Ops 6 (21-bit position off centre, QTangent)", MeshDecode::PositionCodec::Packed21, MeshDecode::NormalCodec::QTangent,
          "ee57c9beb5c14a30", "01eeab0c", "0d362439", 0x41aaf566, { 0xbefc8a00, 0x3e9ac880, 0xbed5d680 },
          { 0xc11623f8, 0xc03300d5, 0xc0b5340c }, { 0x3f24a76a, 0x3e8d1cc5, 0xbf36e28b }, { 0x3ec1a000, 0x3f248000 } },
        { "Modern Warfare 2019 (21-bit position, packed quaternion)", MeshDecode::PositionCodec::Packed21, MeshDecode::NormalCodec::PackedQuat,
          "bd9f1a4b7e550e1d", "c30ebbfe", "043a8d2f", 0x3fc00000, { 0xbf3126e8, 0xba831000, 0x3f51a9fc },
          { 0x3e9ba5ca, 0xbec3127e, 0xb4c00000 }, { 0x3f34d536, 0xbaeb8900, 0x3f35347e }, { 0x3f408000, 0x3df1a000 } },
        { "Cold War (float position, ten-bit normal)", MeshDecode::PositionCodec::Float3, MeshDecode::NormalCodec::Signed10,
          "6eb4c7c215fa19c219430842", "0002f83f", "0036003c", 0x3f800000, { 0, 0, 0 },
          { 0xc2c7b46e, 0xc219fa15, 0x42084319 }, { 0, 0, 0x3f800000 }, { 0x3ec00000, 0x3f800000 } },
        { "Cold War (float position, ten-bit normal down -y)", MeshDecode::PositionCodec::Float3, MeshDecode::NormalCodec::Signed10,
          "000080bf000080bf000080bf", "00060020", "0000003c", 0x3f800000, { 0, 0, 0 },
          { 0xbf800000, 0xbf800000, 0xbf800000 }, { 0, 0xbf800000, 0 }, { 0, 0x3f800000 } },
        { "Modern Warfare 2 (float position, unit vector A, V before U)", MeshDecode::PositionCodec::Float3, MeshDecode::NormalCodec::UnitA,
          "3b3d0a41dd5ca4bf5fb708be", "7f02983f", "e530833b", 0x3f800000, { 0, 0, 0 },
          { 0x410a3d3b, 0xbfa45cdd, 0xbe08b75f }, { 0, 0xbf7bf7f0, 0x3e499326 }, { 0x3f706000, 0x3e1ca000 } },
        { "Black Ops II (float position, unit vector B)", MeshDecode::PositionCodec::Float3, MeshDecode::NormalCodec::UnitB,
          "b3a285c160f366416e872640", "8102e02a", "0d37353b", 0x3f800000, { 0, 0, 0 },
          { 0xc185a2b3, 0x4166f360, 0x4026876e }, { 0xbf3fdff0, 0, 0xbf2954aa }, { 0x3ee1a000, 0x3f66a000 } },
        { "Advanced Warfare (float position, unit vector C)", MeshDecode::PositionCodec::Float3, MeshDecode::NormalCodec::UnitC,
          "af5fbabfbd3786359963ab3f", "130208c0", "f83bf63b", 0x3f800000, { 0, 0, 0 },
          { 0xbfba5faf, 0x358637bd, 0x3fab6399 }, { 0x3d1c2700, 0x3a802000, 0xbf800000 }, { 0x3f7f0000, 0x3f7ec000 } },
    };
}

int MeshDecode::SelfTest(std::string& Failures)
{
    int Failed = 0;
    const auto Check = [&](bool Condition, const std::string& Name, const std::string& Got = {}) {
        if (Condition) return;
        Failed++;
        Failures += std::format("FAIL MeshDecode {} (got '{}')\n", Name, Got);
    };
    const auto Bits = [](float F) { return std::bit_cast<uint32_t>(F); };

    // ---- the vertex codecs, on real vertices: the streams laid one after the other
    for (const auto& R : Vertices)
    {
        auto Buf = FromHex(R.Pos);
        const uint64_t NormalAt = Buf.size();
        for (uint8_t B : FromHex(R.Nrm)) Buf.push_back(B);
        const uint64_t UvAt = Buf.size();
        for (uint8_t B : FromHex(R.Uv)) Buf.push_back(B);
        SubmeshStreams S;
        S.VertexCount = 1;
        S.Positions = { 0, 1 }, S.Normals = { NormalAt, 4 }, S.Uvs = { UvAt, 4 };
        S.Scale = std::bit_cast<float>(R.Scale);
        S.Offset = Vector3(std::bit_cast<float>(R.Offset[0]), std::bit_cast<float>(R.Offset[1]), std::bit_cast<float>(R.Offset[2]));
        // the games of unit vector A store V first
        const Encoding E{ R.Position, R.Normal, R.Normal == NormalCodec::UnitA ? UvCodec::Half2Swapped : UvCodec::Half2, FaceCodec::U16 };
        WraithSubmesh Out;
        Check(DecodeVertices({ Buf.data(), Buf.size() }, S, E, nullptr, Out) && Out.Verticies.size() == 1, std::format("{} decodes", R.What));
        if (Out.Verticies.size() != 1) continue;
        const auto& V = Out.Verticies[0];
        Check(Bits(V.Position.X) == R.P[0] && Bits(V.Position.Y) == R.P[1] && Bits(V.Position.Z) == R.P[2], std::format("{} position", R.What),
              std::format("{} {} {}", V.Position.X, V.Position.Y, V.Position.Z));
        Check(Bits(V.Normal.X) == R.N[0] && Bits(V.Normal.Y) == R.N[1] && Bits(V.Normal.Z) == R.N[2], std::format("{} normal", R.What),
              std::format("{} {} {}", V.Normal.X, V.Normal.Y, V.Normal.Z));
        Check(V.UVLayers.size() == 1 && Bits(V.UVLayers[0].X) == R.U[0] && Bits(V.UVLayers[0].Y) == R.U[1], std::format("{} uv", R.What));
        Check(V.Color[0] == 255 && V.Color[3] == 255 && V.Weights.size() == 1 && V.Weights[0].BoneIndex == 0, std::format("{} white, on bone 0", R.What));

        // the same vertex with its last position byte cut off: the submesh is left empty
        WraithSubmesh Cut;
        const Bytes Short{ Buf.data(), NormalAt - 1 };
        S.Normals.Offset = S.Uvs.Offset = 0;
        Check(!DecodeSubmesh(Short, S, E, nullptr, Cut) && Cut.Verticies.empty(), std::format("{} past the buffer is left empty", R.What));
    }

    // ---- interleaved records with colours, and a stream past the end
    {
        // two 12-byte records: float position, colour
        float P[6] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };
        uint8_t Buf[32]{};
        std::memcpy(Buf, P, 12), std::memcpy(Buf + 16, P + 3, 12);
        Buf[12] = 1, Buf[13] = 2, Buf[14] = 3, Buf[15] = 4;
        Buf[28] = 5, Buf[29] = 6, Buf[30] = 7, Buf[31] = 8;
        SubmeshStreams S;
        S.VertexCount = 2;
        S.Positions = { 0, 16 }, S.Colors = { 12, 16 };
        const Encoding E{ PositionCodec::Float3, NormalCodec::None, UvCodec::None, FaceCodec::U16 };
        WraithSubmesh Out;
        Check(DecodeVertices({ Buf, sizeof(Buf) }, S, E, nullptr, Out) && Out.Verticies.size() == 2, "interleaved records decode");
        if (Out.Verticies.size() == 2)
        {
            Check(Out.Verticies[1].Position.Y == 5.0f && Out.Verticies[1].Color[0] == 5 && Out.Verticies[1].Color[3] == 8,
                  "the second record's position and colour");
            Check(Out.Verticies[0].UVLayers.empty(), "no UV layer when none is read");
        }
        // colours past the end: white, the vertices still read
        S.Colors = { 13, 16 };
        WraithSubmesh White;
        DecodeVertices({ Buf, sizeof(Buf) }, S, E, nullptr, White);
        Check(White.Verticies.size() == 2 && White.Verticies[0].Color[0] == 255, "colours past the end come out white");
        // a third record past the end: nothing read
        S.VertexCount = 3;
        WraithSubmesh None;
        Check(!DecodeVertices({ Buf, sizeof(Buf) }, S, E, nullptr, None) && None.Verticies.empty(), "a vertex past the end reads nothing");
        S.Positions.Offset = NoStream;
        S.VertexCount = 0;
        Check(!VerticesFit({ Buf, sizeof(Buf) }, S, E), "a missing position stream does not fit");
    }

    // ---- faces: u16 triples, the last one cut by the buffer's end
    {
        const uint16_t Idx[8] = { 0, 1, 2, 2, 1, 3, 4, 5 };
        SubmeshStreams S;
        S.FaceCount = 3, S.Faces = 0;
        WraithSubmesh Out;
        Check(!DecodeFaces({ (const uint8_t*)Idx, sizeof(Idx) }, S, {}, Out), "a face past the end stops the faces");
        Check(Out.Faces.size() == 2 && Out.Faces[1].Index1 == 2 && Out.Faces[1].Index3 == 3, "the faces before it are kept",
              std::format("{}", Out.Faces.size()));
    }

    // ---- packed faces: two faces whose corners (2 bits each: 0 1 2, 2 1 3) index a run of u16
    // indices from its second entry (10 + corner)
    const uint8_t Corners[2] = { 0xA4, 0x0D };
    const uint16_t Run[5] = { 100, 10, 11, 12, 13 };
    const auto Expect = [&](const WraithSubmesh& Out, const char* What) {
        Check(Out.Faces.size() >= 2 && Out.Faces[0].Index1 == 10 && Out.Faces[0].Index2 == 11 && Out.Faces[0].Index3 == 12 &&
                  Out.Faces[1].Index1 == 12 && Out.Faces[1].Index2 == 11 && Out.Faces[1].Index3 == 13,
              What, Out.Faces.empty() ? "none" : std::format("{} {} {}", Out.Faces[0].Index1, Out.Faces[0].Index2, Out.Faces[0].Index3));
    };
    {
        // a 40-byte table: first index u32 28, width + 1 (4: values up to 3, 2 bits) byte 34, face
        // count byte 35, corners at u32 36
        uint8_t Buf[40 + 2 + 10]{};
        Buf[28] = 1, Buf[34] = 4, Buf[35] = 2;
        std::memcpy(Buf + 40, Corners, 2);
        std::memcpy(Buf + 42, Run, 10);
        SubmeshStreams S;
        S.FaceCount = 3, S.Tables = 0, S.TableCount = 1, S.Packed = 40, S.Faces = 42;
        const Encoding E{ PositionCodec::Packed21, NormalCodec::QTangent, UvCodec::Half2, FaceCodec::PackedTable40 };
        WraithSubmesh Out;
        Check(DecodeFaces({ Buf, sizeof(Buf) }, S, E, Out), "40-byte tables decode");
        Expect(Out, "40-byte tables");
        Check(Out.Faces.size() == 3 && Out.Faces[2].Index3 == 13, "a face past the tables repeats the last");
        WraithSubmesh Past;
        S.TableCount = 2;
        Check(!DecodeFaces({ Buf, sizeof(Buf) }, S, E, Past) && Past.Faces.empty(), "40-byte tables past the end read nothing");
    }
    {
        // a 32-byte delta table: face count - 1 byte 19, first index u24 20, corner width byte 23
        // (its highest bit: 3 -> 2 bits), corners at u32 24, flags byte 28 (0x10: plain corners)
        uint8_t Buf[32 + 8 + 10]{};
        Buf[19] = 1, Buf[20] = 1, Buf[23] = 3, Buf[28] = 0x10;
        std::memcpy(Buf + 32, Corners, 2);
        std::memcpy(Buf + 40, Run, 10);
        SubmeshStreams S;
        S.FaceCount = 2, S.Tables = 0, S.TableCount = 1, S.Packed = 32, S.Faces = 40;
        const Encoding E{ PositionCodec::Packed21, NormalCodec::QTangent, UvCodec::Half2, FaceCodec::PackedTableDelta, 32, Edge::Partial };
        WraithSubmesh Out;
        Check(DecodeFaces({ Buf, sizeof(Buf) }, S, E, Out), "delta tables decode");
        Expect(Out, "delta tables");
        // a third face past the tables, an index past the end, a table past the end
        S.FaceCount = 3;
        WraithSubmesh Third;
        Check(!DecodeFaces({ Buf, sizeof(Buf) }, S, E, Third) && Third.Faces.size() == 2, "a face past the delta tables stops them");
        uint32_t X[3]{};
        Check(!UnpackDeltaFace(Buf, sizeof(Buf) - 4, 0, 1, 32, 40, 1, X, 32, Edge::Partial), "an index past the end fails the face");
        Check(!UnpackDeltaFace(Buf, 31, 0, 1, 32, 40, 0, X, 32, Edge::Partial), "a table past the end fails the face");
        // the corners at the buffer's very end (their 8-byte word cut): the bytes there (Partial),
        // or 0 (Zero: every corner the run's first)
        uint8_t End[32 + 10 + 2]{};
        std::memcpy(End, Buf, 32);
        std::memcpy(End + 32, Run, 10);
        std::memcpy(End + 42, Corners, 2);
        uint32_t Partial[3]{}, Zero[3]{};
        Check(UnpackDeltaFace(End, sizeof(End), 0, 1, 42, 32, 1, Partial, 32, Edge::Partial) && Partial[0] == 12 && Partial[2] == 13,
              "a cut corner word reads its bytes", std::format("{} {} {}", Partial[0], Partial[1], Partial[2]));
        Check(UnpackDeltaFace(End, sizeof(End), 0, 1, 42, 32, 1, Zero, 32, Edge::Zero) && Zero[0] == 10 && Zero[2] == 10,
              "a cut corner word reads 0", std::format("{} {} {}", Zero[0], Zero[1], Zero[2]));
    }

    // ---- weights
    {
        // one vertex on two bones, column by column: its first bone (5) and padding, then bone 7
        // with 0x4000 (a quarter)
        const uint8_t Data[8] = { 5, 0, 0, 0, 7, 0, 0, 0x40 };
        uint16_t Counts[8]{};
        Counts[1] = 1;
        Check(SkinnedColumnsSize(Counts) == 8, "skinned columns' size");
        std::vector<WeightsData> W(2);
        ReadSkinnedColumns({ Data, sizeof(Data) }, Counts, 1, W);
        Check(W[1].WeightCount == 2 && W[1].BoneValues[0] == 5 && W[1].BoneValues[1] == 7 && W[1].WeightValues[0] == 0.75f &&
                  W[1].WeightValues[1] == 0.25f && W[0].BoneValues[0] == 0,
              "skinned columns", std::format("{} {} {}", W[1].WeightCount, W[1].BoneValues[0], W[1].WeightValues[0]));
        // the second column past the end: the first one only
        std::vector<WeightsData> Short(2);
        ReadSkinnedColumns({ Data, 4 }, Counts, 1, Short);
        Check(Short[1].WeightCount == 1 && Short[1].BoneValues[0] == 5, "skinned columns cut by the end");
        // a vertex past the weights' vector is skipped
        std::vector<WeightsData> One(1);
        ReadSkinnedColumns({ Data, sizeof(Data) }, Counts, 1, One);
        Check(One[0].BoneValues[0] == 0, "skinned columns past the vertices");
    }
    {
        // per-vertex records: weights 255, 0, 128, 0 on bones 1, 2, 3 (bit 15 set), 4
        const uint8_t Rec[12] = { 255, 0, 128, 0, 1, 0, 2, 0, 3, 0x80, 4, 0 };
        std::vector<WeightsData> W;
        uint32_t Max = 0;
        Check(ReadVertexWeights({ Rec, sizeof(Rec) }, 0, 1, 0x7FFF, Max, nullptr, W) && W.size() == 1, "vertex weights read");
        Check(W[0].WeightCount == 2 && W[0].BoneValues[0] == 1 && W[0].BoneValues[1] == 3 && W[0].WeightValues[0] == 1.0f &&
                  W[0].WeightValues[1] == 128 / 255.0f && Max == 3,
              "vertex weights: the zero ones left out, the bone masked", std::format("{} {}", W[0].WeightCount, Max));
        uint32_t Budget = 0;
        Check(ReadVertexWeights({ Rec, sizeof(Rec) }, 0, 1, 0xFFFF, Max, &Budget, W) && W[0].BoneValues[0] == 0 && W[0].WeightCount == 1,
              "no budget left: bone 0");
        Check(!ReadVertexWeights({ Rec, sizeof(Rec) }, 4, 1, 0xFFFF, Max, nullptr, W), "vertex weights past the end");
    }
    return Failed;
}
