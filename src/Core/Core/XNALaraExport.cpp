#include "stdafx.h"

#include "XNALaraExport.h"

#include "WraithModel.h"

#include <format>
#include <fstream>
#include <stdexcept>

void XNALara::ExportXNA(const WraithModel& Model, const std::string& FileName)
{
    std::ofstream Writer(FileName);
    if (!Writer) throw std::runtime_error("cannot create " + FileName);
    Writer.rdbuf()->pubsetbuf(nullptr, 0x100000);   // MSVC: a 1 MB buffer, the file comes in small pieces
    Writer << std::format("{}\n", Model.BoneCount());
    for (auto& Bone : Model.Bones)
    {
        Writer << std::format(
            "{}\n"
            "{}\n"
            "{:f} {:f} {:f}",
            Bone.TagName.c_str(),
            Bone.BoneParent,
            Bone.GlobalPosition.X, Bone.GlobalPosition.Y, Bone.GlobalPosition.Z) << '\n';
    }
    auto SubmeshCount = Model.SubmeshCount();
    Writer << std::format("{}\n", SubmeshCount);
    uint32_t SubmeshIndex = 0;
    for (auto& Submesh : Model.Submeshes)
    {
        Writer << std::format(
            "WraithMesh{:02}\n"
            "1\n1",
            SubmeshIndex) << '\n';
        const WraithMaterial& Material = (Submesh.MaterialIndicies[0] > -1) ? Model.Materials[Submesh.MaterialIndicies[0]] : WraithMaterial::DefaultMaterial;
        Writer << std::format(
            "{}\n"
            "0\n"
            "{}",
            Material.MaterialName.c_str(),
            Submesh.VertexCount()) << '\n';
        for (auto& Vertex : Submesh.Verticies)
        {
            Writer << std::format(
                "{:f} {:f} {:f}\n"
                "{:f} {:f} {:f}\n"
                "255 255 255 255\n"
                "{:f} {:f}",
                Vertex.Position.X, Vertex.Position.Y, Vertex.Position.Z,
                Vertex.Normal.X, Vertex.Normal.Y, Vertex.Normal.Z,
                Vertex.UVLayers[0].U, Vertex.UVLayers[0].V) << '\n';
            uint32_t WeightIDs[4] = { 0, 0, 0, 0 };
            float WeightValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            // Loop and apply (Only the first 4 weights are supported)
            for (uint32_t w = 0; w < std::min<uint32_t>(Vertex.WeightCount(), 4); w++)
            {
                WeightIDs[w] = Vertex.Weights[w].BoneIndex;
                WeightValues[w] = Vertex.Weights[w].Weight;
            }
            Writer << std::format(
                "{} {} {} {}\n"
                "{:f} {:f} {:f} {:f}",
                WeightIDs[0], WeightIDs[1], WeightIDs[2], WeightIDs[3],
                WeightValues[0], WeightValues[1], WeightValues[2], WeightValues[3]) << '\n';
        }
        Writer << std::format("{}\n", Submesh.FacesCount());
        for (auto& Face : Submesh.Faces)
        {
            Writer << std::format("{} {} {}\n", Face.Index1, Face.Index2, Face.Index3);
        }
        SubmeshIndex++;
    }
}