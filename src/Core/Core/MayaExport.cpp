#include "stdafx.h"

#include "MayaExport.h"

#include "Hashing.h"
#include "FileSystems.h"
#include "Strings.h"
#include "WraithModel.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <ostream>
#include <stdexcept>

static const char* GenericSettings = "createNode transform -s -n \"persp\";\n\tsetAttr \".v\" no;\n\tsetAttr \".t\" -type \"double3\" 48.186233840145825 37.816674066853686 41.0540421364379 ;\n\tsetAttr \".r\" -type \"double3\" -29.738352729603015 49.400000000000432 0 ;\ncreateNode camera -s -n \"perspShape\" -p \"persp\";\n\tsetAttr -k off \".v\" no;\n\tsetAttr \".fl\" 34.999999999999993;\n\tsetAttr \".fcp\" 10000;\n\tsetAttr \".coi\" 73.724849603665149;\n\tsetAttr \".imn\" -type \"string\" \"persp\";\n\tsetAttr \".den\" -type \"string\" \"persp_depth\";\n\tsetAttr \".man\" -type \"string\" \"persp_mask\";\n\tsetAttr \".hc\" -type \"string\" \"viewSet -p %camera\";\ncreateNode transform -s -n \"top\";\n\tsetAttr \".v\" no;\n\tsetAttr \".t\" -type \"double3\" 0 100.1 0 ;\n\tsetAttr \".r\" -type \"double3\" -89.999999999999986 0 0 ;\ncreateNode camera -s -n \"topShape\" -p \"top\";\n\tsetAttr -k off \".v\" no;\n\tsetAttr \".rnd\" no;\n\tsetAttr \".coi\" 100.1;\n\tsetAttr \".ow\" 30;\n\tsetAttr \".imn\" -type \"string\" \"top\";\n\tsetAttr \".den\" -type \"string\" \"top_depth\";\n\tsetAttr \".man\" -type \"string\" \"top_mask\";\n\tsetAttr \".hc\" -type \"string\" \"viewSet -t %camera\";\n\tsetAttr \".o\" yes;\ncreateNode transform -s -n \"front\";\n\tsetAttr \".v\" no;\n\tsetAttr \".t\" -type \"double3\" 0 0 100.1 ;\ncreateNode camera -s -n \"frontShape\" -p \"front\";\n\tsetAttr -k off \".v\" no;\n\tsetAttr \".rnd\" no;\n\tsetAttr \".coi\" 100.1;\n\tsetAttr \".ow\" 30;\n\tsetAttr \".imn\" -type \"string\" \"front\";\n\tsetAttr \".den\" -type \"string\" \"front_depth\";\n\tsetAttr \".man\" -type \"string\" \"front_mask\";\n\tsetAttr \".hc\" -type \"string\" \"viewSet -f %camera\";\n\tsetAttr \".o\" yes;\ncreateNode transform -s -n \"side\";\n\tsetAttr \".v\" no;\n\tsetAttr \".t\" -type \"double3\" 100.1 0 0 ;\n\tsetAttr \".r\" -type \"double3\" 0 89.999999999999986 0 ;\ncreateNode camera -s -n \"sideShape\" -p \"side\";\n\tsetAttr -k off \".v\" no;\n\tsetAttr \".rnd\" no;\n\tsetAttr \".coi\" 100.1;\n\tsetAttr \".ow\" 30;\n\tsetAttr \".imn\" -type \"string\" \"side\";\n\tsetAttr \".den\" -type \"string\" \"side_depth\";\n\tsetAttr \".man\" -type \"string\" \"side_mask\";\n\tsetAttr \".hc\" -type \"string\" \"viewSet -s %camera\";\n\tsetAttr \".o\" yes;\ncreateNode lightLinker -n \"lightLinker1\";\n\tsetAttr -s 9 \".lnk\";\n\tsetAttr -s 9 \".slnk\";\ncreateNode displayLayerManager -n \"layerManager\";\ncreateNode displayLayer -n \"defaultLayer\";\ncreateNode renderLayerManager -n \"renderLayerManager\";\ncreateNode renderLayer -n \"defaultRenderLayer\";\n\tsetAttr \".g\" yes;\ncreateNode script -n \"sceneConfigurationScriptNode\";\n\tsetAttr \".b\" -type \"string\" \"playbackOptions -min 1 -max 24 -ast 1 -aet 48 \";\n\tsetAttr \".st\" 6;\nselect -ne :time1;\n\tsetAttr \".o\" 1;\nselect -ne :renderPartition;\n\tsetAttr -s 2 \".st\";\nselect -ne :renderGlobalsList1;\nselect -ne :defaultShaderList1;\n\tsetAttr -s 2 \".s\";\nselect -ne :postProcessList1;\n\tsetAttr -s 2 \".p\";\nselect -ne :lightList1;\nselect -ne :initialShadingGroup;\n\tsetAttr \".ro\" yes;\nselect -ne :initialParticleSE;\n\tsetAttr \".ro\" yes;\nselect -ne :hardwareRenderGlobals;\n\tsetAttr \".ctrs\" 256;\n\tsetAttr \".btrs\" 512;\nselect -ne :defaultHardwareRenderGlobals;\n\tsetAttr \".fn\" -type \"string\" \"im\";\n\tsetAttr \".res\" -type \"string\" \"ntsc_4d 646 485 1.333\";\nselect -ne :ikSystem;\n\tsetAttr -s 4 \".sol\";\nconnectAttr \":defaultLightSet.msg\" \"lightLinker1.lnk[0].llnk\";\nconnectAttr \":initialShadingGroup.msg\" \"lightLinker1.lnk[0].olnk\";\nconnectAttr \":defaultLightSet.msg\" \"lightLinker1.lnk[1].llnk\";\nconnectAttr \":initialParticleSE.msg\" \"lightLinker1.lnk[1].olnk\";\nconnectAttr \":defaultLightSet.msg\" \"lightLinker1.slnk[0].sllk\";\nconnectAttr \":initialShadingGroup.msg\" \"lightLinker1.slnk[0].solk\";\nconnectAttr \":defaultLightSet.msg\" \"lightLinker1.slnk[1].sllk\";\nconnectAttr \":initialParticleSE.msg\" \"lightLinker1.slnk[1].solk\";\nconnectAttr \"layerManager.dli[0]\" \"defaultLayer.id\";\nconnectAttr \"renderLayerManager.rlmi[0]\" \"defaultRenderLayer.rlid\";\nconnectAttr \"lightLinker1.msg\" \":lightList1.ln\" -na;\n";

bool SortByWeightValue(uint32_t i, uint32_t j) { return (i < j); }

// The vertices one skinPercent weighs, sorted, runs of consecutive indices as [first:last]
static void WriteVertexRuns(std::ostream& Writer, const std::string& HashName, uint32_t SubmeshIndex, std::vector<uint32_t>& Vertices)
{
    std::sort(Vertices.begin(), Vertices.end(), SortByWeightValue);
    auto WeightLen = Vertices.size();
    for (size_t i = 0; i < WeightLen; i++)
    {
        Writer << std::format(" WraithMesh_{}_{:02}.vtx[{}", HashName.c_str(), SubmeshIndex, Vertices[i]);
        size_t j = i;
        for (; j < WeightLen; j++)
        {
            if ((j + 1) >= WeightLen || (Vertices[j] + 1) != Vertices[j + 1])
            {
                break;
            }
        }
        if (i == j)
        {
            Writer << "]";
        }
        else
        {
            Writer << std::format(":{}]", Vertices[j]);
        }
        i = j;
    }
    Writer << ";\n";
}

void Maya::ExportMaya(const WraithModel& Model, const std::string& FileName)
{
    auto HashName = Hashing::HashCRC32String(Model.AssetName);
    {
        std::ofstream Writer(FileName);
        if (!Writer) throw std::runtime_error("cannot create " + FileName);
        Writer.rdbuf()->pubsetbuf(nullptr, 0x100000);   // MSVC: a 1 MB buffer, the file comes in small pieces
        Writer << "//Maya ASCII 8.5 scene\n\n// Wraith - Game extraction tools\n";
        Writer << "// Please credit DTZxPorter for using it!\n\n";
        Writer << "requires maya \"8.5\";\ncurrentUnit -l centimeter -a degree -t film;\nfileInfo \"application\" \"maya\";\nfileInfo \"product\" \"Maya Unlimited 8.5\";\nfileInfo \"version\" \"8.5\";\nfileInfo \"cutIdentifier\" \"200612162224-692032\";\n";
        Writer << GenericSettings;
        Writer << std::format("createNode transform -n \"{}\";\n", Model.AssetName.c_str());
        Writer << "setAttr \".ove\" yes;\n";
        uint32_t SubmeshIndex = 0;
        for (auto& Submesh : Model.Submeshes)
        {
            Writer << std::format("createNode transform -n \"WraithMesh_{}_{:02}\" -p \"{}\";\n", HashName.c_str(), SubmeshIndex, Model.AssetName.c_str());
            Writer << std::format("setAttr \".rp\" -type \"double3\" 0.000000 0.000000 0.000000 ;\nsetAttr \".sp\" -type \"double3\" 0.000000 0.000000 0.000000 ;\n");
            Writer << std::format("createNode mesh -n \"MeshShape_{}\" -p \"WraithMesh_{}_{:02}\";\n", SubmeshIndex, HashName.c_str(), SubmeshIndex);
            Writer << "setAttr -k off \".v\";\nsetAttr \".vir\" yes;\nsetAttr \".vif\" yes;\n";
            Writer << std::format("setAttr \".iog[0].og[0].gcl\" -type \"componentList\" 1 \"f[0:{}]\";\n", Submesh.VertexCount());
            Writer << std::format("setAttr \".uvst[0].uvsn\" -type \"string\" \"map1\";\nsetAttr -s {} \".uvst[0].uvsp\";\n", Submesh.VertexCount());

            if (Submesh.VertexCount() == 1)
                Writer << "setAttr \".uvst[0].uvsp[0]\" -type \"float2\"";
            else
                Writer << std::format("setAttr \".uvst[0].uvsp[0:{}]\" -type \"float2\"", (Submesh.VertexCount() - 1));
            for (auto& Vertex : Submesh.Verticies)
            {
                // Write it, for now, we are only applying the initial UV layer
                Writer << std::format(" {:f} {:f}", Vertex.UVLayers[0].U, (1 - Vertex.UVLayers[0].V));
            }
            Writer << ";\n";

            Writer << "setAttr \".cuvs\" -type \"string\" \"map1\";\nsetAttr \".dcol\" yes;\nsetAttr \".dcc\" -type \"string\" \"Ambient+Diffuse\";\nsetAttr \".ccls\" -type \"string\" \"colorSet1\";\nsetAttr \".clst[0].clsn\" -type \"string\" \"colorSet1\";\n";
            Writer << std::format("setAttr -s {} \".clst[0].clsp\";\n", (Submesh.FacesCount() * 3));

            Writer << std::format("setAttr \".clst[0].clsp[0:{}]\"", ((Submesh.FacesCount() * 3) - 1));
            for (auto& Face : Submesh.Faces)
            {
                Writer << std::format(" {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f}",
                    // The the verts, 3->2->1
                    Submesh.Verticies[Face.Index3].Color[0] / 255.0f, Submesh.Verticies[Face.Index3].Color[1] / 255.0f, Submesh.Verticies[Face.Index3].Color[2] / 255.0f, Submesh.Verticies[Face.Index3].Color[3] / 255.0f,
                    Submesh.Verticies[Face.Index2].Color[0] / 255.0f, Submesh.Verticies[Face.Index2].Color[1] / 255.0f, Submesh.Verticies[Face.Index2].Color[2] / 255.0f, Submesh.Verticies[Face.Index2].Color[3] / 255.0f,
                    Submesh.Verticies[Face.Index1].Color[0] / 255.0f, Submesh.Verticies[Face.Index1].Color[1] / 255.0f, Submesh.Verticies[Face.Index1].Color[2] / 255.0f, Submesh.Verticies[Face.Index1].Color[3] / 255.0f
                );
            }
            Writer << ";\n";

            Writer << std::format("setAttr \".covm[0]\"  0 1 1;\nsetAttr \".cdvm[0]\"  0 1 1;\nsetAttr -s {} \".vt\";\n", Submesh.VertexCount());

            if (Submesh.VertexCount() == 1)
                Writer << "setAttr \".vt[0]\"";
            else
                Writer << std::format("setAttr \".vt[0:{}]\"", (Submesh.VertexCount() - 1));
            for (auto& Vertex : Submesh.Verticies)
            {
                Writer << std::format(" {:f} {:f} {:f}", Vertex.Position.X, Vertex.Position.Y, Vertex.Position.Z);
            }
            Writer << ";\n";

            Writer << std::format("setAttr -s {} \".ed\";\n", (Submesh.FacesCount() * 3));

            Writer << std::format("setAttr \".ed[0:{}]\"", ((Submesh.FacesCount() * 3) - 1));
            for (auto& Face : Submesh.Faces)
            {
                // Output edge combinations, 3 per face 3->2 2->1 1->3
                Writer << std::format(" {} {} 0 {} {} 0 {} {} 0", Face.Index3, Face.Index2, Face.Index2, Face.Index1, Face.Index1, Face.Index3);
            }
            Writer << ";\n";

            Writer << std::format("setAttr -s {} \".n\";\n", (Submesh.FacesCount() * 3));

            Writer << std::format("setAttr \".n[0:{}]\" -type \"float3\"", ((Submesh.FacesCount() * 3) - 1));
            for (auto& Face : Submesh.Faces)
            {
                Writer << std::format(" {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f}",
                    // The the verts, 3->2->1
                    Submesh.Verticies[Face.Index3].Normal.X, Submesh.Verticies[Face.Index3].Normal.Y, Submesh.Verticies[Face.Index3].Normal.Z,
                    Submesh.Verticies[Face.Index2].Normal.X, Submesh.Verticies[Face.Index2].Normal.Y, Submesh.Verticies[Face.Index2].Normal.Z,
                    Submesh.Verticies[Face.Index1].Normal.X, Submesh.Verticies[Face.Index1].Normal.Y, Submesh.Verticies[Face.Index1].Normal.Z
                    );
            }
            Writer << ";\n";

            if (Submesh.FacesCount() == 1)
                Writer << std::format("setAttr -s {} \".fc[0]\" -type \"polyFaces\"", Submesh.FacesCount());
            else
                Writer << std::format("setAttr -s {} \".fc[0:{}]\" -type \"polyFaces\"", Submesh.FacesCount(), (Submesh.FacesCount() - 1));
            {
                uint32_t FaceIndex = 0;
                for (auto& Face : Submesh.Faces)
                {
                    // Output face connections, 3 per face, index, uv, index
                    Writer << std::format(" f 3 {} {} {} mu 0 3 {} {} {} mc 0 3 {} {} {}",
                        // The indicies 1->2->3
                        FaceIndex, (FaceIndex + 1), (FaceIndex + 2),
                        Face.Index3, Face.Index2, Face.Index1,
                        FaceIndex, (FaceIndex + 1), (FaceIndex + 2)
                        );
                    FaceIndex += 3;
                }
                Writer << ";\n";
            }
            Writer << "setAttr \".cd\" -type \"dataPolyComponent\" Index_Data Edge 0 ;\nsetAttr \".cvd\" -type \"dataPolyComponent\" Index_Data Vertex 0 ;\nsetAttr \".hfd\" -type \"dataPolyComponent\" Index_Data Face 0 ;\n";
            SubmeshIndex++;
        }
        Writer << '\n';
        std::unordered_set<int32_t> MaterialIndicies;
        for (auto& Submesh : Model.Submeshes)
        {
            if (MaterialIndicies.insert(Submesh.MaterialIndicies[0]).second)
            {
                const WraithMaterial& Material = (Submesh.MaterialIndicies[0] > -1) ? Model.Materials[Submesh.MaterialIndicies[0]] : WraithMaterial::DefaultMaterial;
                Writer << std::format("createNode shadingEngine -n \"{}SG\";\n    setAttr \".ihi\" 0;\n    setAttr \".ro\" yes;\n", Material.MaterialName.c_str());
                Writer << std::format("createNode materialInfo -n \"{}MI\";\r\ncreateNode phong -n \"{}\";\n", Material.MaterialName.c_str(), Material.MaterialName.c_str());
                Writer << "    setAttr \".ambc\" -type \"float3\" 1 1 1 ;\n";
                if (!Strings::IsNullOrWhiteSpace(Material.DiffuseMapName))
                {
                    Writer << std::format("createNode file -n \"{}FILE\";\n    setAttr \".ftn\" -type \"string\" \"{}\";\n", Material.MaterialName.c_str(), Material.DiffuseMapName.c_str());
                }
                Writer << std::format("createNode place2dTexture -n \"{}P2DT\";\n\n", Material.MaterialName.c_str());
            }
        }
        uint32_t LightConnectionIndex = 2;
        for (auto& MaterialIndex : MaterialIndicies)
        {
            const WraithMaterial& Material = (MaterialIndex > -1) ? Model.Materials[MaterialIndex] : WraithMaterial::DefaultMaterial;
            Writer << std::format(
                "connectAttr \":defaultLightSet.msg\" \"lightLinker1.lnk[{}].llnk\";\n"
                "connectAttr \"{}SG.msg\" \"lightLinker1.lnk[{}].olnk\";\n"
                "connectAttr \":defaultLightSet.msg\" \"lightLinker1.slnk[{}].sllk\";\n"
                "connectAttr \"{}SG.msg\" \"lightLinker1.slnk[{}].solk\";\n"
                "connectAttr \"{}.oc\" \"{}SG.ss\";\n"
                "connectAttr \"{}SG.msg\" \"{}MI.sg\";\n"
                "connectAttr \"{}.msg\" \"{}MI.m\";",
                LightConnectionIndex,
                Material.MaterialName.c_str(), LightConnectionIndex,
                LightConnectionIndex,
                Material.MaterialName.c_str(), LightConnectionIndex,
                Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                Material.MaterialName.c_str(), Material.MaterialName.c_str()
                ) << '\n';
            if (!Strings::IsNullOrWhiteSpace(Material.DiffuseMapName))
            {
                Writer << std::format(
                    "connectAttr \"{}FILE.msg\" \"{}MI.t\" -na;\n"
                    "connectAttr \"{}FILE.oc\" \"{}.c\";\n"
                    "connectAttr \"{}P2DT.c\" \"{}FILE.c\";\n"
                    "connectAttr \"{}P2DT.tf\" \"{}FILE.tf\";\n"
                    "connectAttr \"{}P2DT.rf\" \"{}FILE.rf\";\n"
                    "connectAttr \"{}P2DT.mu\" \"{}FILE.mu\";\n"
                    "connectAttr \"{}P2DT.mv\" \"{}FILE.mv\";\n"
                    "connectAttr \"{}P2DT.s\" \"{}FILE.s\";\n"
                    "connectAttr \"{}P2DT.wu\" \"{}FILE.wu\";\n"
                    "connectAttr \"{}P2DT.wv\" \"{}FILE.wv\";\n"
                    "connectAttr \"{}P2DT.re\" \"{}FILE.re\";\n"
                    "connectAttr \"{}P2DT.of\" \"{}FILE.of\";\n"
                    "connectAttr \"{}P2DT.r\" \"{}FILE.ro\";\n"
                    "connectAttr \"{}P2DT.n\" \"{}FILE.n\";\n"
                    "connectAttr \"{}P2DT.vt1\" \"{}FILE.vt1\";\n"
                    "connectAttr \"{}P2DT.vt2\" \"{}FILE.vt2\";\n"
                    "connectAttr \"{}P2DT.vt3\" \"{}FILE.vt3\";\n"
                    "connectAttr \"{}P2DT.vc1\" \"{}FILE.vc1\";\n"
                    "connectAttr \"{}P2DT.o\" \"{}FILE.uv\";\n"
                    "connectAttr \"{}P2DT.ofs\" \"{}FILE.fs\";\n",
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str(),
                    Material.MaterialName.c_str(), Material.MaterialName.c_str()
                    ) << '\n';
            }
            Writer << std::format(
                "connectAttr \"{}SG.pa\" \":renderPartition.st\" -na;\n"
                "connectAttr \"{}.msg\" \":defaultShaderList1.s\" -na;\n"
                "connectAttr \"{}P2DT.msg\" \":defaultRenderUtilityList1.u\" -na;\n"
                "connectAttr \"{}FILE.msg\" \":defaultTextureList1.tx\" -na;\n",
                Material.MaterialName.c_str(),
                Material.MaterialName.c_str(),
                Material.MaterialName.c_str(),
                Material.MaterialName.c_str()
                ) << '\n';
            LightConnectionIndex++;
        }
        uint32_t SubmeshMaterialOffset = 0;
        for (auto& Submesh : Model.Submeshes)
        {
            const WraithMaterial& Material = (Submesh.MaterialIndicies[0] > -1) ? Model.Materials[Submesh.MaterialIndicies[0]] : WraithMaterial::DefaultMaterial;
            Writer << std::format(
                "connectAttr \"{}SG.mwc\" \"MeshShape_{}.iog.og[0].gco\";\n"
                "connectAttr \"MeshShape_{}.iog\" \"{}SG.dsm\" -na;",
                Material.MaterialName.c_str(), SubmeshMaterialOffset,
                SubmeshMaterialOffset, Material.MaterialName.c_str()
                ) << '\n';
            SubmeshMaterialOffset++;
        }
        Writer << "createNode transform -n \"Joints\";\nsetAttr \".ove\" yes;\n\n";
        for (auto& Bone : Model.Bones)
        {
            if (Bone.BoneParent == -1)
            {
                Writer << std::format("createNode joint -n \"{}\" -p \"Joints\";\n", Bone.TagName.c_str());
            }
            else
            {
                Writer << std::format("createNode joint -n \"{}\" -p \"{}\";\n", Bone.TagName.c_str(), Model.Bones[Bone.BoneParent].TagName.c_str());
            }
            Vector3 LocalRotation = Bone.LocalRotation.ToEulerAngles();
            Writer << std::format(
                "addAttr -ci true -sn \"liw\" -ln \"lockInfluenceWeights\" -bt \"lock\" -min 0 -max 1 -at \"bool\";\nsetAttr \".uoc\" yes;\nsetAttr \".ove\" yes;\n"
                "setAttr \".t\" -type \"double3\"  {:f} {:f} {:f} ;\n"
                "setAttr \".mnrl\" -type \"double3\" -360 -360 -360 ;\nsetAttr \".mxrl\" -type \"double3\" 360 360 360 ;\nsetAttr \".radi\"   0.50;\n"
                "setAttr \".jo\" -type \"double3\" {:f} {:f} {:f};\n"
                "setAttr \".scale\" -type \"double3\" {:f} {:f} {:f};\n",
                Bone.LocalPosition.X, Bone.LocalPosition.Y, Bone.LocalPosition.Z,
                LocalRotation.X, LocalRotation.Y, LocalRotation.Z,
                Bone.BoneScale.X, Bone.BoneScale.Y, Bone.BoneScale.Z
                ) << '\n';
        }
    }
    {
        const auto BindPath = (std::filesystem::path(FileSystems::GetDirectoryName(FileName)) / (FileSystems::GetFileNameWithoutExtension(FileName) + "_BIND.mel")).string();
        std::ofstream Writer(BindPath);
        if (!Writer) throw std::runtime_error("cannot create " + BindPath);
        Writer.rdbuf()->pubsetbuf(nullptr, 0x100000);   // MSVC: a 1 MB buffer, the file comes in small pieces
        Writer << "/*\n* Advanced weighting script X\n* Wraith - Game extraction tools\n*/\n\n";
        uint32_t SubmeshIndex = 0;
        for (auto& Submesh : Model.Submeshes)
        {
            Writer << std::format(
                "global proc WraithMesh_{}_{:02}_BindFunc()\n{{\n"
                "   select -r WraithMesh_{}_{:02};",
                HashName.c_str(), SubmeshIndex,
                HashName.c_str(), SubmeshIndex
                ) << '\n';
            std::unordered_map<std::string, std::vector<uint32_t>> SimpleBoneWeights;
            std::unordered_map<std::string, std::unordered_map<float, std::vector<uint32_t>>> ComplexBoneWeights;
            std::unordered_set<std::string> UniqueBoneNames;
            uint32_t VertexIndex = 0;
            for (auto& Vertex : Submesh.Verticies)
            {
                uint32_t Index = 0;
                for (auto& Weight : Vertex.Weights)
                {
                    auto BoneID = (Weight.BoneIndex == -1) ? 0 : Weight.BoneIndex;
                    if (Index == 0)
                    {
                        SimpleBoneWeights[Model.Bones[BoneID].TagName].push_back(VertexIndex);
                    }
                    else
                    {
                        ComplexBoneWeights[Model.Bones[BoneID].TagName][Weight.Weight].push_back(VertexIndex);
                    }
                    UniqueBoneNames.insert(Model.Bones[BoneID].TagName);
                    Index++;
                }
                VertexIndex++;
            }
            for (auto& Bone : UniqueBoneNames)
            {
                Writer << std::format("   select -add {};\n", Bone.c_str());
            }
            Writer << std::format(
                "   newSkinCluster \"-toSelectedBones -mi 15 -omi true -dr 5.0 -rui false\";"
                "   string $clu = findRelatedSkinCluster(\"WraithMesh_{}_{:02}\");\n",
                HashName.c_str(), SubmeshIndex
                ) << '\n';
            for (auto& SimpleWeights : SimpleBoneWeights)
            {
                Writer << std::format("   skinPercent -tv {} 1.0 $clu", SimpleWeights.first.c_str());
                WriteVertexRuns(Writer, HashName, SubmeshIndex, SimpleWeights.second);
            }
            for (auto& ComplexWeights : ComplexBoneWeights)
            {
                for (auto& WeightSet : ComplexWeights.second)
                {
                    Writer << std::format("   skinPercent -tv {} {:f} $clu", ComplexWeights.first.c_str(), WeightSet.first);
                    WriteVertexRuns(Writer, HashName, SubmeshIndex, WeightSet.second);
                }
            }
            Writer << "}\n\n";
            SubmeshIndex++;
        }
        Writer << "global proc RunAdvancedScript()\n{\n";
        for (size_t SubmeshIndex = 0; SubmeshIndex < Model.SubmeshCount(); SubmeshIndex++)
        {
            Writer << std::format("   catch(WraithMesh_{}_{:02}_BindFunc());\n", HashName.c_str(), SubmeshIndex);
        }
        Writer << "}\n\nglobal proc NamespacePurge()\n{\n   string $allNodes[] = `ls`;\n   for($node in $allNodes) {\n      string $buffer[];\n      tokenize $node \":\" $buffer;\n      string $newName = $buffer[size($buffer)-1];\n       catchQuiet(`rename $node $newName`);\n   }\n}\n\nprint(\"Currently binding the current model, please wait...\\n\");\nNamespacePurge();\nRunAdvancedScript();\nprint(\"The model has been binded.\\n\");\n\n";
    }
}