#include "stdafx.h"

#include "CoDXModelTranslator.h"

#include "MemoryReader.h"
#include "HalfFloats.h"
#include "Strings.h"

#include "ICoDGame.h"

#include "DBGameGenerics.h"
#include "SettingsManager.h"
#include "Games/Common/MeshDecode.h"

#include <algorithm>
#include <format>

namespace
{
    // Then the skinned ones, in Groups: WeightCounts[g] vertices on g + 1 bones each, stored as
    // their first bone then g (bone, u16 weight) pairs, Padding bytes after each vertex
    void ReadSkinnedWeights(std::vector<WeightsData>& Weights, uint32_t& Index, const XModelSubmesh_t& Submesh, uint32_t Groups, uint32_t BoneScale,
                            uint32_t Padding)
    {
        size_t Length = 0;
        for (uint32_t g = 0; g < Groups; g++)
            Length += (size_t)(2 + 4 * g + Padding) * Submesh.WeightCounts[g];

        uintptr_t ReadDataSize = 0;
        auto Data = MemoryReader(CoDAssets::GameInstance->Read(Submesh.WeightsPtr, Length, ReadDataSize), Length);

        for (uint32_t g = 0; g < Groups; g++)
        {
            for (uint32_t w = 0; w < Submesh.WeightCounts[g]; w++)
            {
                auto& Weight = Weights[Index++];
                Weight.BoneValues[0] = Data.Read<uint16_t>() / BoneScale;

                if (g > 0)
                {
                    Weight.WeightCount = (uint8_t)(g + 1);
                    for (uint32_t k = 1; k <= g; k++)
                    {
                        Weight.BoneValues[k] = Data.Read<uint16_t>() / BoneScale;
                        Weight.WeightValues[k] = ((float)Data.Read<uint16_t>() / 65536.0f);
                    }

                    float Others = Weight.WeightValues[1];
                    for (uint32_t k = 2; k <= g; k++)
                        Others += Weight.WeightValues[k];
                    Weight.WeightValues[0] = 1.0f - Others;
                }

                if (Padding > 0)
                    Data.Advance(Padding);
            }
        }
    }

    // The weights of a surface the zone holds whole, by the game's packing: A (WAW, BO, BO2, MW,
    // MW2, MW3) 32-bit rigid runs and four groups; B (Ghosts, AW, MWR) eight groups; C (IW) eight
    // groups of raw bone indices padded to 4 bytes
    void PrepareVertexWeights(std::vector<WeightsData>& Weights, const XModelSubmesh_t& Submesh, VertexWeightsVariant Variant)
    {
        uint32_t Index = 0;
        switch (Variant)
        {
        case VertexWeightsVariant::A:
            Index = MeshDecode::ReadRigidRuns(Submesh, { sizeof(GfxRigidVerts), 2, 64 }, Weights);
            ReadSkinnedWeights(Weights, Index, Submesh, 4, 64, 0);
            break;
        case VertexWeightsVariant::B:
            Index = MeshDecode::ReadRigidRuns(Submesh, { sizeof(GfxRigidVerts64), 2, 64 }, Weights);
            ReadSkinnedWeights(Weights, Index, Submesh, 8, 64, 0);
            break;
        case VertexWeightsVariant::C:
            Index = MeshDecode::ReadRigidRuns(Submesh, { sizeof(GfxRigidVerts64), 2, 1 }, Weights);
            ReadSkinnedWeights(Weights, Index, Submesh, 8, 1, 2);
            break;
        default:
            break;
        }
    }
}

std::unique_ptr<WraithModel> CoDXModelTranslator::TranslateXModel(const std::unique_ptr<XModel_t>& Model, uint32_t LodIndex, bool JustBones)
{
    bool ExportColors = (SettingsManager::GetSetting("exportvtxcolor", "true") == "true");
    auto ModelResult = std::make_unique<WraithModel>();

    ModelResult->AssetName = Model->ModelName;

    ModelResult->LodDistance = Model->ModelLods[LodIndex].LodDistance;
    ModelResult->LodMaxDistance = Model->ModelLods[LodIndex].LodMaxDistance;

    if (CoDAssets::Game == nullptr || !CoDAssets::Game->ReadsOwnBones())
    {
        uintptr_t ReadDataSize = 0;
        auto GlobalMatrixLength = (sizeof(DObjAnimMat) * (((uint64_t)Model->BoneCount + Model->CosmeticBoneCount)));
        auto LocalTranslationLength = (sizeof(Vector3) * (((uint64_t)Model->BoneCount + Model->CosmeticBoneCount) - Model->RootBoneCount));
        auto LocalRotationLength = (sizeof(QuatData) * (((uint64_t)Model->BoneCount + Model->CosmeticBoneCount) - Model->RootBoneCount));
        auto GlobalMatrixData = MemoryReader(CoDAssets::GameInstance->Read(Model->BaseMatriciesPtr, GlobalMatrixLength, ReadDataSize), GlobalMatrixLength);
        auto LocalTranslationData = MemoryReader(CoDAssets::GameInstance->Read(Model->TranslationsPtr, LocalTranslationLength, ReadDataSize), LocalTranslationLength);
        auto LocalRotationData = MemoryReader(CoDAssets::GameInstance->Read(Model->RotationsPtr, LocalRotationLength, ReadDataSize), LocalRotationLength);
        bool NeedsLocalPositions = true;

        auto BoneIDs = Model->BoneIDsPtr;
        auto BoneParents = Model->BoneParentsPtr;

        ModelResult->PrepareBones(Model->BoneCount + Model->CosmeticBoneCount);

        for (uint32_t i = 0; i < (Model->BoneCount + Model->CosmeticBoneCount); i++)
        {
            uint64_t BoneID = 0;

            switch (Model->BoneIndexSize)
            {
            case 2: BoneID = (uint64_t)CoDAssets::GameInstance->Read<uint16_t>(BoneIDs); break;
            case 4: BoneID = (uint64_t)CoDAssets::GameInstance->Read<uint32_t>(BoneIDs); break;
            case 8: BoneID = (uint64_t)CoDAssets::GameInstance->Read<uint32_t>(BoneIDs + 4); break;
            }

            auto& NewBone = ModelResult->AddBone();

            std::string BoneName;

            switch (Model->BoneIndexSize)
            {
            case 2:
            case 4:
                BoneName = CoDAssets::GameStringHandler(BoneID);
                break;
            case 8:
                BoneName = CoDAssets::GetHashedString("bone", BoneID);
                break;
            }

            if (BoneName == "")
            {
                if (i == 0)
                {
                    NewBone.TagName = "tag_origin";
                }
                else
                {
                    NewBone.TagName = std::format("no_tag_{}", i);
                }
            }
            else
            {
                NewBone.TagName = BoneName;
            }

            int32_t BoneParent = -1;

            if (i >= Model->RootBoneCount)
            {
                switch (Model->BoneParentSize)
                {
                case 1: BoneParent = CoDAssets::GameInstance->Read<uint8_t>(BoneParents); break;
                case 2: BoneParent = CoDAssets::GameInstance->Read<uint16_t>(BoneParents); break;
                case 4: BoneParent = CoDAssets::GameInstance->Read<uint32_t>(BoneParents); break;
                }

                if (i < Model->BoneCount)
                {
                    // Subtract position
                    BoneParent = (i - BoneParent);
                }
                else
                {
                    // Absolute position
                    BoneParent = BoneParent;
                    NewBone.IsCosmetic = true;
                }

                BoneParents += Model->BoneParentSize;
            }
            else
            {
                BoneParent = (i - 1);
            }

            NewBone.BoneParent = BoneParent;

            auto GlobalData = GlobalMatrixData.Read<DObjAnimMat>();

            NewBone.GlobalPosition = GlobalData.Translation;
            NewBone.GlobalRotation = GlobalData.Rotation;

            if (i > (Model->RootBoneCount - 1))
            {
                NewBone.LocalPosition = LocalTranslationData.Read<Vector3>();

                if (NewBone.LocalPosition != Vector3(0, 0, 0))
                {
                    NeedsLocalPositions = false;
                }

                auto RotationData = LocalRotationData.Read<QuatData>();
                if (Model->BoneRotationData == BoneDataTypes::DivideBySize)
                {
                    NewBone.LocalRotation = Quaternion((float)RotationData.RotationX / 32768.0f, (float)RotationData.RotationY / 32768.0f, (float)RotationData.RotationZ / 32768.0f, (float)RotationData.RotationW / 32768.0f);
                }
                else if (Model->BoneRotationData == BoneDataTypes::HalfFloat)
                {
                    NewBone.LocalRotation = Quaternion(HalfFloats::ToFloat((uint16_t)RotationData.RotationX), HalfFloats::ToFloat((uint16_t)RotationData.RotationY), HalfFloats::ToFloat((uint16_t)RotationData.RotationZ), HalfFloats::ToFloat((uint16_t)RotationData.RotationW));
                }
                else if (Model->BoneRotationData == BoneDataTypes::QuatPackingA)
                {
                    NewBone.LocalRotation = VectorPacking::QuatPackingA(*(uint64_t*)&RotationData);
                }
            }

            BoneIDs += Model->BoneIndexSize;
        }

        GlobalMatrixData.Close();
        LocalTranslationData.Close();
        LocalRotationData.Close();

        // If we're a viewmodel (!UseBones) && we have at least > 1 bone, we need to generate local positions from the globals
        if (NeedsLocalPositions && Model->BoneCount > 1)
        {
            ModelResult->GenerateLocalPositions(true, false);
        }

        // Check if we didn't parse any bones, if we didn't, we need can "inject" tag_origin (this is mostly for MW which has weird models
        // that are rare with thousands of bones)
        if (Model->BoneCount == 0)
        {
            auto& NewBone = ModelResult->AddBone();
            NewBone.TagName = "tag_origin";
            NewBone.BoneParent = -1;
        }
    }

    // Check if we just wanted the bones of the model (Used for hitbox logic)
    if (JustBones) { return ModelResult; }

    const ICoDGame* CurrentGame = CoDAssets::Game;

    auto& LodReference = Model->ModelLods[LodIndex];

    std::map<std::string, uint32_t> UniqueMaterials;

    // Build unique material references (Right now, it's one material per submesh)
    for (uint32_t i = 0; i < (uint32_t)LodReference.Materials.size(); i++)
    {
        auto FindResult = UniqueMaterials.find(LodReference.Materials[i].MaterialName);
        if (FindResult != UniqueMaterials.end())
        {
            LodReference.Submeshes[i].MaterialIndex = UniqueMaterials[LodReference.Materials[i].MaterialName];
        }
        else
        {
            auto& NewMat = ModelResult->AddMaterial();
            NewMat.MaterialName = Strings::Replace(LodReference.Materials[i].MaterialName, "@", "_");

            for (auto& Image : LodReference.Materials[i].Images)
            {
                switch (Image.ImageUsage)
                {
                case ImageUsageType::DiffuseMap:
                    NewMat.DiffuseMapName = Image.ImageName;
                    break;
                case ImageUsageType::NormalMap:
                    NewMat.NormalMapName = Image.ImageName;
                    break;
                case ImageUsageType::SpecularMap:
                    NewMat.SpecularMapName = Image.ImageName;
                    break;
                }
            }

            LodReference.Submeshes[i].MaterialIndex = (uint32_t)(UniqueMaterials.size());

            UniqueMaterials.insert(std::make_pair(LodReference.Materials[i].MaterialName, LodReference.Submeshes[i].MaterialIndex));
        }
    }

    if (Model->IsModelStreamed)
    {
        // We have a streamed model, this is handled on a per-game basis, some information is already in the structures
        if (auto* Adapter = CurrentGame)
        {
            Adapter->LoadStreamedXModel(Model, LodReference, ModelResult);
        }
    }
    else
    {
        // the zone holds the surface whole: GfxVertexBuffer records (a position, a colour at 16, half
        // UVs at 20, a packed normal at 24, by the game's variant) and u16 faces
        const auto Variant = CurrentGame ? CurrentGame->GetNormalUnpackVariant() : NormalUnpackVariant::None;
        MeshDecode::Encoding Encoding{ MeshDecode::PositionCodec::Float3, MeshDecode::NormalCodec::None, MeshDecode::UvCodec::None, MeshDecode::FaceCodec::U16 };
        switch (Variant)
        {
        // These games seem to have UVV before UVU
        case NormalUnpackVariant::A: Encoding.Normal = MeshDecode::NormalCodec::UnitA, Encoding.Uv = MeshDecode::UvCodec::Half2Swapped; break;
        case NormalUnpackVariant::B: Encoding.Normal = MeshDecode::NormalCodec::UnitB, Encoding.Uv = MeshDecode::UvCodec::Half2; break;
        case NormalUnpackVariant::C: Encoding.Normal = MeshDecode::NormalCodec::UnitC, Encoding.Uv = MeshDecode::UvCodec::Half2; break;
        default: break;
        }

        ModelResult->PrepareSubmeshes((uint32_t)LodReference.Submeshes.size());

        for (auto& Submesh : LodReference.Submeshes)
        {
            auto& Mesh = ModelResult->AddSubmesh();

            // Set the material (COD has 1 per submesh)
            Mesh.AddMaterial(Submesh.MaterialIndex);

            Mesh.PrepareMesh(Submesh.VertexCount, Submesh.FaceCount);

            // Pre-allocate vertex weights (Data defaults to weight 1.0 on bone 0)
            auto VertexWeights = std::vector<WeightsData>(Submesh.VertexCount);

            PrepareVertexWeights(VertexWeights, Submesh, CurrentGame ? CurrentGame->GetVertexWeightsVariant() : VertexWeightsVariant::None);

            // the records as far as the zone holds them, zero past it
            const uint64_t VerticesLength = sizeof(GfxVertexBuffer) * (uint64_t)Submesh.VertexCount, FacesLength = sizeof(GfxFaceBuffer) * (uint64_t)Submesh.FaceCount;
            const auto Vertices = MeshDecode::FromZone(Submesh.VertexPtr, VerticesLength);
            const auto Faces = MeshDecode::FromZone(Submesh.FacesPtr, FacesLength);
            MeshDecode::SubmeshStreams Streams;
            Streams.VertexCount = Submesh.VertexCount, Streams.FaceCount = Submesh.FaceCount;
            Streams.Positions = { 0, sizeof(GfxVertexBuffer) };
            if (ExportColors) Streams.Colors = { 16, sizeof(GfxVertexBuffer) };
            Streams.Uvs = { 20, sizeof(GfxVertexBuffer) };
            Streams.Normals = { 24, sizeof(GfxVertexBuffer) };
            Streams.Faces = 0;
            MeshDecode::DecodeVertices({ Vertices.Data.get(), Vertices.Data ? VerticesLength : 0 }, Streams, Encoding, VertexWeights.data(), Mesh);
            MeshDecode::DecodeFaces({ Faces.Data.get(), Faces.Data ? FacesLength : 0 }, Streams, Encoding, Mesh);
        }
    }

    return ModelResult;
}

int32_t CoDXModelTranslator::CalculateBiggestLodIndex(const std::unique_ptr<XModel_t>& Model)
{
    auto LodCount = (int32_t)Model->ModelLods.size();

    // This function is only valid if we have > 1 lod
    if (LodCount > 1)
    {
        int32_t ResultIndex = 0;

        for (int32_t i = 0; i < LodCount; i++)
        {
            if ((Model->ModelLods[i].LodDistance < Model->ModelLods[ResultIndex].LodDistance) && (Model->ModelLods[i].Submeshes.size() >= Model->ModelLods[ResultIndex].Submeshes.size()))
            {
                ResultIndex = i;
            }
        }
        
        return ResultIndex;
    }
    else if (LodCount == 0)
    {
        // There are no lods to export
        return -1;
    }

    return 0;
}

Vector3 CoDXModelTranslator::UnpackNormalA(uint32_t Normal)
{
    // Unpack a normal, used in [WAW, BO, MW, MW2, MW3]

    auto PackedNormal = (GfxPackedUnitVec*)&Normal;

    float DecodeScale = (float)((float)(uint8_t)PackedNormal->PackedBytes[3] - -192.0) / 32385.0f;

    return Vector3(
        (float)((float)(uint8_t)PackedNormal->PackedBytes[0] - 127.0) * DecodeScale,
        (float)((float)(uint8_t)PackedNormal->PackedBytes[1] - 127.0) * DecodeScale,
        (float)((float)(uint8_t)PackedNormal->PackedBytes[2] - 127.0) * DecodeScale);
}

Vector3 CoDXModelTranslator::UnpackNormalB(uint32_t Normal)
{
    // Unpack a normal, used in [BO2]

    auto PackedNormal = (GfxPackedUnitVec*)&Normal;

    // Rebuild the float (As an integer value)
    uint32_t RBuiltX = ((PackedNormal->PackedInteger & 0x3FF) - 2 * (PackedNormal->PackedInteger & 0x200) + 0x40400000);
    uint32_t RBuiltY = (((PackedNormal->PackedInteger >> 10) & 0x3FF) - 2 * ((PackedNormal->PackedInteger >> 10) & 0x200) + 0x40400000);
    uint32_t RBuiltZ = (((PackedNormal->PackedInteger >> 20) & 0x3FF) - 2 * ((PackedNormal->PackedInteger >> 20) & 0x200) + 0x40400000);

    return Vector3(
        (float)(*reinterpret_cast<float*>(&RBuiltX) - 3.0) * 8208.0312f,
        (float)(*reinterpret_cast<float*>(&RBuiltY) - 3.0) * 8208.0312f,
        (float)(*reinterpret_cast<float*>(&RBuiltZ) - 3.0) * 8208.0312f);
}

Vector3 CoDXModelTranslator::UnpackNormalC(uint32_t Normal)
{
    // Unpack a normal, used in [Ghosts, AW, MWR, IW]

    auto PackedNormal = (GfxPackedUnitVec*)&Normal;

    return Vector3(
        (float)((float)((float)(PackedNormal->PackedInteger & 0x3FF) / 1023.0) * 2.0) - 1.0f,
        (float)((float)((float)((PackedNormal->PackedInteger >> 10) & 0x3FF) / 1023.0) * 2.0) - 1.0f,
        (float)((float)((float)((PackedNormal->PackedInteger >> 20) & 0x3FF) / 1023.0) * 2.0) - 1.0f);
}

std::unique_ptr<WraithModel> CoDXModelTranslator::TranslateXModelHitbox(const std::unique_ptr<XModel_t>& Model)
{
    if (Model->BoneInfoPtr == 0) { return nullptr; }

    auto Result = TranslateXModel(Model, 0, true);

    if (Result != nullptr)
    {
        auto BoneInfoPtr = Model->BoneInfoPtr;

        for (size_t i = 0; i < (Result->Bones.size()); i++)
        {
            auto BoneInfo = CoDAssets::GameInstance->Read<GfxBoneInfo>(BoneInfoPtr);

            if (std::equal(std::begin(BoneInfo.Bounds[0]), std::end(BoneInfo.Bounds[0]), std::begin(BoneInfo.Bounds[1]), std::end(BoneInfo.Bounds[1])) == false)
            {
                auto& NewMesh = Result->AddSubmesh();
                NewMesh.PrepareMesh(8, 12);
                NewMesh.AddMaterial(-1);

                std::vector<Vector3> VertPos =
                {
                    { BoneInfo.Bounds[0][0], BoneInfo.Bounds[0][1], BoneInfo.Bounds[0][2] },
                    { BoneInfo.Bounds[0][0], BoneInfo.Bounds[0][1], BoneInfo.Bounds[1][2] },
                    { BoneInfo.Bounds[0][0], BoneInfo.Bounds[1][1], BoneInfo.Bounds[0][2] },
                    { BoneInfo.Bounds[0][0], BoneInfo.Bounds[1][1], BoneInfo.Bounds[1][2] },
                    { BoneInfo.Bounds[1][0], BoneInfo.Bounds[0][1], BoneInfo.Bounds[0][2] },
                    { BoneInfo.Bounds[1][0], BoneInfo.Bounds[0][1], BoneInfo.Bounds[1][2] },
                    { BoneInfo.Bounds[1][0], BoneInfo.Bounds[1][1], BoneInfo.Bounds[0][2] },
                    { BoneInfo.Bounds[1][0], BoneInfo.Bounds[1][1], BoneInfo.Bounds[1][2] }
                };

                auto& Bone = Result->Bones[i];

                auto Matrix = Matrix::CreateFromQuaternion(Bone.GlobalRotation);
                Matrix.Mat(3, 0) = Bone.GlobalPosition.X;
                Matrix.Mat(3, 1) = Bone.GlobalPosition.Y;
                Matrix.Mat(3, 2) = Bone.GlobalPosition.Z;

                for (auto& Vertex : VertPos)
                {
                    auto& Vert = NewMesh.AddVertex();
                    Vert.Position = Matrix::TransformVector(Vertex, Matrix);
                    Vert.AddUVLayer(0.5, 0.5);
                    Vert.Normal = Vector3(0.2f, 0.2f, 0.2f);
                    Vert.AddVertexWeight((uint32_t)i, 1.0f);
                }

                NewMesh.AddFace(0, 2, 1);
                NewMesh.AddFace(1, 2, 3);
                NewMesh.AddFace(4, 5, 6);
                NewMesh.AddFace(6, 5, 7);
                NewMesh.AddFace(0, 1, 4);
                NewMesh.AddFace(4, 1, 5);
                NewMesh.AddFace(2, 6, 3);
                NewMesh.AddFace(3, 6, 7);
                NewMesh.AddFace(2, 0, 6);
                NewMesh.AddFace(6, 0, 4);
                NewMesh.AddFace(5, 1, 7);
                NewMesh.AddFace(7, 1, 3);
            }

            BoneInfoPtr += sizeof(GfxBoneInfo);
        }

        return Result;
    }

    return nullptr;
}