#include "stdafx.h"

// BO3 xcam (84) -> its xcam.gdf entry and its .xcam_export (JSON), as HydraX's XCam.cs and XCamObj.cs
// (github.com/Scobalula/HydraX, GPL-3.0): header (0xa8) {name 0, parent scene 8, 7 flag bytes 0x10,
// camera count 0x18, frames 0x1c, notetrack count 0x24, root bone 0x48, cameras 0x50, switches 0x58
// (one per frame), notetracks 0x60, right stick offset 0x94, max degrees 0xa0}; a camera (0x20)
// {name 0, index 4, frame count 8, frames 0x10, far z 0x18}; a frame (0x30) {origin 4, quaternion
// 0x10, fov, focal length, focus distance, f-stop 0x20..0x2c}. The file goes in type.jsonc's "files"
// folder, named in the entry under its "filename" prefix. HydraX reads every frame of camera i from
// frame i and leaves each camera's index 0: here each frame is its own, the index the camera's.

#include "Gdt.h"
#include "HydraText.h"

#include <algorithm>
#include <array>

namespace
{
    using HydraText::Json;

    using Row = std::array<double, 3>;
    struct Matrix
    {
        Row X{}, Y{}, Z{};
    };

    // HydraX's Quaternion.ToMatrix: each value rounded to 4 places
    Matrix ToMatrix(double X, double Y, double Z, double W)
    {
        const auto R = HydraText::Round4;
        Matrix M;
        const double Xx = X * X, Yy = Y * Y, Zz = Z * Z, Ww = W * W;
        const double Inv = 1 / (Xx + Yy + Zz + Ww);
        M.X[0] = R((Xx - Yy - Zz + Ww) * Inv);
        M.Y[1] = R((-Xx + Yy - Zz + Ww) * Inv);
        M.Z[2] = R((-Xx - Yy + Zz + Ww) * Inv);
        M.Y[0] = R(2.0 * (X * Y + Z * W) * Inv);
        M.X[1] = R(2.0 * (X * Y - Z * W) * Inv);
        M.Z[0] = R(2.0 * (X * Z - Y * W) * Inv);
        M.X[2] = R(2.0 * (X * Z + Y * W) * Inv);
        M.Z[1] = R(2.0 * (Y * Z + X * W) * Inv);
        M.Y[2] = R(2.0 * (Y * Z - X * W) * Inv);
        return M;
    }

    Json Vec(const Row& V)
    {
        Json J = Json::Array();
        for (double X : V) J.Push(Json::Num(X));
        return J;
    }
    Json Axis(const Row& X, const Row& Y, const Row& Z)
    {
        Json J;
        J.Set("x", Vec(X));
        J.Set("y", Vec(Y));
        J.Set("z", Vec(Z));
        return J;
    }

    // a camera or one of its frames: where it is, its axes, its lens
    struct View
    {
        Row Origin{}, Dir{}, Up{}, Right{};
        double Flen = 27.0, Fov = 28.7985, Fdist = 109.4973, Fstop = 1.2;
        void Write(Json& J) const
        {
            J.Set("origin", Vec(Origin));
            J.Set("dir", Vec(Dir));
            J.Set("up", Vec(Up));
            J.Set("right", Vec(Right));
            J.Set("flen", Json::Num(Flen));
            J.Set("fov", Json::Num(Fov));
            J.Set("fdist", Json::Num(Fdist));
            J.Set("fstop", Json::Num(Fstop));
            J.Set("lense", Json::Int(10));
        }
    };

    bool Convert(const Gdt::Type& T, const Gdt::Memory& M, int64_t Pos, const std::string& Name, std::vector<Gdt::Block>& Out, Gdt::Stats& S)
    {
        const auto Count = [&](int64_t At) { return std::clamp(M.At<int32_t>(Pos + At), 0, 1 << 20); };
        const std::string Files = T.Settings().value("files", std::string());
        const std::string Prefix = T.Settings().value("filename", std::string());

        Gdt::Block B{ Name, "xcam.gdf", "", {} };
        const char* const FlagKeys[] = { "hide_hud", "is_looping", "hide_local_player", "use_firstperson_player",
                                         "disableNearDof", "autoMotionBlur", "easeAnimationsOut" };
        for (int I = 0; I < 7; I++) B.Keys.emplace_back(FlagKeys[I], std::to_string(M.At<uint8_t>(Pos + 0x10 + I)));
        const char* const StickKeys[] = { "rightStickRotateOffsetX", "rightStickRotateOffsetY", "rightStickRotateOffsetZ",
                                          "rightStickRotateMaxDegreesX", "rightStickRotateMaxDegreesY" };
        for (int I = 0; I < 5; I++) B.Keys.emplace_back(StickKeys[I], HydraText::Float(M.At<float>(Pos + 0x94 + 4 * I)));
        B.Keys.emplace_back("parent_scene", M.StringField(Pos + 8));
        B.Keys.emplace_back("filename", Prefix + Name + ".xcam_export");
        Out.push_back(std::move(B));

        Json File;
        File.Set("version", Json::Int(1));
        File.Set("scene", Json::Str("exported_via_HydraX_by_Scobalula.fbx"));
        {
            Json Align;
            Align.Set("tag", Json::Str("tag_align"));
            Align.Set("offset", Vec({}));
            Align.Set("axis", Axis({}, {}, {}));
            File.Set("align", std::move(Align));
        }
        File.Set("framerate", Json::Int(30));
        File.Set("numframes", Json::Int(0));   // the most frames a camera has, set below

        // the target model's root bone (0x10) {name 0, frame count 4, frames 8 (0x1c: origin 0,
        // quaternion 0xc)}, its Y and Z mirrored as HydraX does
        Json Roots = Json::Array();
        if (const int64_t Bone = M.Follow(Pos + 0x48); Bone >= 0)
        {
            Json Root;
            Root.Set("name", Json::Str(M.ScriptString(M.At<uint32_t>(Bone))));
            Root.Set("axis", Axis({}, {}, {}));
            Json Frames = Json::Array();
            const int64_t List = M.Follow(Bone + 8);
            const int32_t N = std::clamp(M.At<int32_t>(Bone + 4), 0, 1 << 20);
            for (int32_t I = 0; List >= 0 && I < N; I++)
            {
                const int64_t F = List + 0x1c * (int64_t)I;
                const auto V = [&](int K) { return (double)M.At<float>(F + 4 * (int64_t)K); };
                const Matrix Mx = ToMatrix(V(3), -V(4), -V(5), V(6));
                Json Fr;
                Fr.Set("frame", Json::Int(I));
                Fr.Set("offset", Vec({ V(0), -V(1), V(2) }));
                Fr.Set("axis", Axis(Mx.X, Mx.Y, Mx.Z));
                Frames.Push(std::move(Fr));
            }
            Root.Set("animation", std::move(Frames));
            Roots.Push(std::move(Root));
        }
        File.Set("targetModelBoneRoots", std::move(Roots));

        int32_t FrameCount = 0;
        Json Cameras = Json::Array();
        const int64_t CamList = M.Follow(Pos + 0x50);
        for (int32_t I = 0; CamList >= 0 && I < Count(0x18); I++)
        {
            const int64_t C = CamList + 0x20 * (int64_t)I;
            View First;
            Json Frames = Json::Array();
            const int64_t List = M.Follow(C + 0x10);
            const int32_t N = std::clamp(M.At<int32_t>(C + 8), 0, 1 << 20);
            for (int32_t K = 0; List >= 0 && K < N; K++)
            {
                const int64_t F = List + 0x30 * (int64_t)K;
                const auto V = [&](int At) { return (double)M.At<float>(F + At); };
                const Matrix Mx = ToMatrix(V(0x10), V(0x14), V(0x18), V(0x1c));
                View Fv;
                Fv.Dir = { Mx.X[0], Mx.Y[0], Mx.Z[0] };
                Fv.Up = { Mx.X[2], Mx.Y[2], Mx.Z[2] };
                Fv.Right = { Mx.X[1] * -1, Mx.Y[1] * -1, Mx.Z[1] * -1 };
                Fv.Origin = { V(4), V(8), V(0xc) };
                Fv.Fov = V(0x20);
                Fv.Flen = V(0x24);
                Fv.Fdist = V(0x28);
                Fv.Fstop = V(0x2c);
                Json Fr;
                Fr.Set("frame", Json::Int(K));
                Fv.Write(Fr);
                Frames.Push(std::move(Fr));
                if (K == 0) First = Fv;
                FrameCount = std::max(FrameCount, K + 1);
            }
            Json Cam;
            Cam.Set("name", Json::Str(M.ScriptString(M.At<uint32_t>(C))));
            Cam.Set("index", Json::Int(M.At<int32_t>(C + 4)));
            Cam.Set("type", Json::Str("Perspective"));
            Cam.Set("aperture", Json::Str("FOCAL_LENGTH"));
            First.Write(Cam);
            Cam.Set("aspectratio", Json::Num(1.7786));
            Cam.Set("nearz", Json::Num(3.9370));
            Cam.Set("farz", Json::Num(M.At<float>(C + 0x18)));
            Cam.Set("animation", std::move(Frames));
            Cameras.Push(std::move(Cam));
        }
        File.Set("numframes", Json::Int(FrameCount));
        File.Set("cameras", std::move(Cameras));

        // a switch (0x10) {frame 0, cameras 4 and 8, dissolve 0xc}, one per frame when the list is set
        Json Switches = Json::Array();
        if (M.At<uint64_t>(Pos + 0x58) != 0)
        {
            const int64_t List = M.Follow(Pos + 0x58);
            for (int32_t I = 0; List >= 0 && I < Count(0x1c); I++)
            {
                const int64_t W = List + 0x10 * (int64_t)I;
                Json Sw;
                Sw.Set("frame", Json::Int(M.At<int32_t>(W)));
                Sw.Set("dissolve", Json::Num(M.At<int32_t>(W + 0xc)));
                Json Pair = Json::Array();
                Pair.Push(Json::Int(M.At<int32_t>(W + 4)));
                Pair.Push(Json::Int(M.At<int32_t>(W + 8)));
                Sw.Set("cameras", std::move(Pair));
                Switches.Push(std::move(Sw));
            }
        }
        File.Set("cameraSwitch", std::move(Switches));

        // a notetrack (8) {name 0, frame 4}
        Json Notes = Json::Array();
        const int64_t NoteList = M.Follow(Pos + 0x60);
        for (int32_t I = 0; NoteList >= 0 && I < Count(0x24); I++)
        {
            Json Nt;
            Nt.Set("name", Json::Str(M.ScriptString(M.At<uint32_t>(NoteList + 8 * (int64_t)I))));
            Nt.Set("frame", Json::Int(M.At<int32_t>(NoteList + 8 * (int64_t)I + 4)));
            Notes.Push(std::move(Nt));
        }
        File.Set("notetracks", std::move(Notes));

        S.Files.emplace_back(Files + Name + ".xcam_export", HydraText::Dump(File));
        return true;
    }
}

GDT_CONVERTER(t7_xcam, Convert);
