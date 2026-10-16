#include "stdafx.h"

#include "CastExport.h"

#include "BinaryWriter.h"

#include "Hashing.h"
#include "WraithModel.h"

#include <algorithm>
#include <type_traits>

void WriteCastProperty(BinaryWriter& Writer, const std::string& Name, const CastProperty* Prop)
{
	Writer.Write((uint16_t)Prop->Identifier);
	Writer.Write((uint16_t)Name.size());
	Writer.Write((uint32_t)Prop->Elements);
	Writer.Write((const uint8_t*)Name.c_str(), (uint32_t)Name.size());
	if(Prop->Elements > 0)
		Writer.Write((const uint8_t*)&Prop->Buffer[0], (uint32_t)Prop->Buffer.size());
}

void WriteCastNode(BinaryWriter& Writer, const CastNode* Node)
{
	Writer.Write((uint32_t)Node->Identifier);
	Writer.Write((uint32_t)Node->Size());
	Writer.Write(Node->Hash);
	Writer.Write((uint32_t)Node->Properties.size());
	Writer.Write((uint32_t)Node->Children.size());

	for (auto& Prop : Node->Properties)
	{
		WriteCastProperty(Writer, Prop.first, Prop.second.get());
	}

	for (auto& Child : Node->Children)
	{
		WriteCastNode(Writer, Child.get());
	}
}

void WriteCastFile(BinaryWriter& Writer, const CastNode& Node)
{
	Writer.Write(0x74736163);
	Writer.Write(1);
	Writer.Write(1);
	Writer.Write(0);

	WriteCastNode(Writer, &Node);
}

CastPropertyId CastIntegerType(uint64_t Max)
{
	return Max <= 0xFF ? CastPropertyId::Byte : Max <= 0xFFFF ? CastPropertyId::Short : CastPropertyId::Integer32;
}

void CastWriteInteger(CastProperty& Prop, CastPropertyId Type, uint32_t Value)
{
	switch (Type)
	{
	case CastPropertyId::Byte: Prop.Write((uint8_t)Value); break;
	case CastPropertyId::Short: Prop.Write((uint16_t)Value); break;
	default: Prop.Write(Value); break;
	}
}

CastMeshBuilder::CastMeshBuilder(CastNode& Model, uint64_t Hash, size_t Vertices, uint32_t UVLayers,
	uint32_t MaxInfluence, size_t Bones)
	: Mesh(Model.AddNode(CastNodeId::Mesh, Hash)), Indices(CastIntegerType(Vertices))
{
	Positions = Mesh->AddProperty("vp", CastPropertyId::Vector3, Vertices * sizeof(Vector3));
	Normals = Mesh->AddProperty("vn", CastPropertyId::Vector3, Vertices * sizeof(Vector3));
	Colors = Mesh->AddProperty("c0", CastPropertyId::Integer32, Vertices * sizeof(uint32_t));
	Faces = Mesh->AddProperty("f", Indices, Vertices * sizeof(uint32_t));
	for (uint32_t Layer = 0; Layer < UVLayers; Layer++)
		UVs.push_back(Mesh->AddProperty("u" + std::to_string(Layer), CastPropertyId::Vector2, Vertices * sizeof(Vector2)));
	Mesh->SetProperty("ul", CastPropertyId::Byte, (uint8_t)UVLayers);
	Mesh->SetProperty("cl", CastPropertyId::Byte, (uint8_t)1);
	if (MaxInfluence > 0)
	{
		BoneIndices = CastIntegerType(Bones);
		WeightBones = Mesh->AddProperty("wb", BoneIndices);
		WeightValues = Mesh->AddProperty("wv", CastPropertyId::Float, Vertices * MaxInfluence * sizeof(float));
		Mesh->SetProperty("mi", CastPropertyId::Byte, (uint8_t)MaxInfluence);
	}
}

void CastMeshBuilder::Vertex(const Vector3& Position, const Vector3& Normal, uint32_t Color, const Vector2* UVLayers)
{
	Positions->Write(Position);
	Normals->Write(Normal);
	Colors->Write(Color);
	for (size_t Layer = 0; Layer < UVs.size(); Layer++)
		UVs[Layer]->Write(UVLayers[Layer]);
}

void CastMeshBuilder::Weight(uint32_t Bone, float Value)
{
	CastWriteInteger(*WeightBones, BoneIndices, Bone);
	WeightValues->Write(Value);
}

void CastMeshBuilder::Face(uint32_t A, uint32_t B, uint32_t C)
{
	// a degenerate triangle is left out (Maya refuses them)
	if (A == B || B == C || C == A)
		return;
	for (const uint32_t I : { C, B, A })
		CastWriteInteger(*Faces, Indices, I);
}

void CastMeshBuilder::Material(uint64_t Hash)
{
	Mesh->SetProperty("m", CastPropertyId::Integer64, Hash);
}

bool Cast::WriteFile(const CastNode& Root, const std::string& FileName)
{
	BinaryWriter Writer;
	if (!Writer.Create(FileName))
		return false;
	WriteCastFile(Writer, Root);
	return true;
}

void Cast::AddMetadata(CastNode& Root, const std::string& SceneRoot)
{
	// the software and CoD's up axis: a hint the importers may use to stand the scene upright
	auto Meta = Root.AddNode(CastNodeId::Metadata, Hashing::HashXXHashString("metadata"));
	Meta->SetProperty("s", "GrayWolf");
	Meta->SetProperty("up", "z");
	if (!SceneRoot.empty())
		Meta->SetProperty("sr", SceneRoot);
}

CastNode* Cast::AddMaterial(CastNode& Model, const CastMaterialSlots& Material)
{
	auto CastMaterial = Model.AddNode(CastNodeId::Material, Hashing::HashXXHashString(Material.Name));
	CastMaterial->SetProperty("n", Material.Name);
	CastMaterial->SetProperty("t", "pbr");

	// one File node per distinct path (a slot may share another's image); an empty path is no slot
	std::map<std::string, uint64_t> Files;
	for (const auto& [Slot, Path] : Material.Files)
	{
		if (Path.empty())
			continue;
		auto Known = Files.find(Path);
		if (Known == Files.end())
		{
			auto File = CastMaterial->AddNode(CastNodeId::File, Hashing::HashXXHashString(Material.Name + "|" + Path));
			File->SetProperty("p", Path);
			Known = Files.emplace(Path, File->Hash).first;
		}
		CastMaterial->SetProperty(Slot, CastPropertyId::Integer64, Known->second);
	}
	return CastMaterial;
}

void Cast::ExportCastModel(const WraithModel& Model, const std::string& FileName, bool SupportsScale,
	const std::vector<CastMaterialSlots>* Slots)
{
	BinaryWriter Writer;

	if (!Writer.Create(FileName))
		return;

	CastNode CastRoot;
	auto CastModel = CastRoot.AddNode(CastNodeId::Model);
	AddMetadata(CastRoot);
	auto CastSkeleton = CastModel->AddNode(CastNodeId::Skeleton);

	for (auto& Bone : Model.Bones)
	{
		auto CastBone = CastSkeleton->AddNode(CastNodeId::Bone);

		CastBone->SetProperty("n", Bone.TagName);
		CastBone->SetProperty("p", CastPropertyId::Integer32, (uint32_t)Bone.BoneParent);
		CastBone->SetProperty("lr", CastPropertyId::Vector4, Bone.LocalRotation);
		CastBone->SetProperty("lp", CastPropertyId::Vector3, Bone.LocalPosition);
		CastBone->SetProperty("wr", CastPropertyId::Vector4, Bone.GlobalRotation);
		CastBone->SetProperty("wp", CastPropertyId::Vector3, Bone.GlobalPosition);
		CastBone->SetProperty("s", CastPropertyId::Vector3, Bone.BoneScale);
	}

	std::vector<uint64_t> MaterialHashes;

	for (auto& Material : Model.Materials)
	{
		// the caller's slots for this material (its description), else the diffuse/normal/specular triple
		const CastMaterialSlots* Described = nullptr;
		if (Slots != nullptr)
			for (const auto& S : *Slots)
				if (S.Name == Material.MaterialName) { Described = &S; break; }

		CastMaterialSlots Triple;
		if (Described == nullptr)
		{
			Triple.Name = Material.MaterialName;
			Triple.Files = { { "albedo", Material.DiffuseMapName }, { "normal", Material.NormalMapName },
				{ "specular", Material.SpecularMapName } };
		}
		auto CastMaterial = AddMaterial(*CastModel, Described ? *Described : Triple);
		MaterialHashes.push_back(CastMaterial->Hash);
	}

	int MeshIndex = 0;
	for (auto& Mesh : Model.Submeshes)
	{
		auto MeshHash = Hashing::HashXXHashString(MeshIndex == 0 ? "CastMesh" : "CastMesh" + std::to_string(MeshIndex));
		MeshIndex++;

		// every UV layer the vertices carry (a second set for lightmaps or detail maps), and the
		// largest weight count of a vertex
		uint32_t UVLayerCount = 1, MaxInfluence = 0;
		for (auto& Vertex : Mesh.Verticies)
		{
			UVLayerCount = std::max<uint32_t>(UVLayerCount, Vertex.UVLayerCount());
			MaxInfluence = std::max<uint32_t>(MaxInfluence, Vertex.WeightCount());
		}

		CastMeshBuilder Builder(*CastModel, MeshHash, Mesh.VertexCount(), UVLayerCount, MaxInfluence, Model.BoneCount());
		auto CastMesh = Builder.Node();
		auto FaceIndexType = Builder.IndexType();
		std::vector<Vector2> UVs(UVLayerCount);
		for (auto& Vertex : Mesh.Verticies)
		{
			for (uint32_t Layer = 0; Layer < UVLayerCount; Layer++)
				UVs[Layer] = Layer < Vertex.UVLayerCount() ? Vertex.UVLayers[Layer] : Vector2(0, 0);
			Builder.Vertex(Vertex.Position, Vertex.Normal, *(uint32_t*)&Vertex.Color[0], UVs.data());
			for (uint32_t i = 0; i < MaxInfluence; i++)
			{
				const bool Has = i < Vertex.WeightCount();
				Builder.Weight(Has ? Vertex.Weights[i].BoneIndex : 0, Has ? Vertex.Weights[i].Weight : 0.0f);
			}
		}
		for (auto& Face : Mesh.Faces)
			Builder.Face(Face.Index1, Face.Index2, Face.Index3);

		if (Model.BlendShapes.size() > 0)
		{
			std::unordered_map<size_t, CastNode*> blendMap;
			for (int i = 0; i < Mesh.VertexCount(); i++)
			{
				auto& Vertex = Mesh.Verticies[i];
				for (auto& BlendDeltaPosition : Vertex.BlendShapeDeltas)
				{
					if (BlendDeltaPosition.second != Vector3(0, 0, 0))
					{
						if (blendMap.find(BlendDeltaPosition.first) == blendMap.end())
						{
							auto blend = CastModel->AddNode(CastNodeId::BlendShape);
							blend->SetProperty("n", Model.BlendShapes[BlendDeltaPosition.first]);
							blend->SetProperty("b", CastPropertyId::Integer64, CastMesh->Hash);
							blend->SetProperty("ts", CastPropertyId::Float, 1.0f);
							blend->AddProperty("vp", CastPropertyId::Vector3, Vertex.BlendShapeDeltas.size() * sizeof(Vector3));
							blend->AddProperty("vi", FaceIndexType, Vertex.BlendShapeDeltas.size() * sizeof(uint32_t));
							blendMap[BlendDeltaPosition.first] = blend;
						}
						auto& blend = blendMap[BlendDeltaPosition.first];

						blend->Properties["vp"]->Write(Vertex.Position + BlendDeltaPosition.second);
						CastWriteInteger(*blend->Properties["vi"], FaceIndexType, (uint32_t)i);
					}
				}
			}
		}
		// the mesh's first material (SEModel's layer 0); a mesh without one (-1) links none
		if (!Mesh.MaterialIndicies.empty() && Mesh.MaterialIndicies[0] >= 0
			&& (size_t)Mesh.MaterialIndicies[0] < MaterialHashes.size())
			Builder.Material(MaterialHashes[Mesh.MaterialIndicies[0]]);
	}

	WriteCastFile(Writer, CastRoot);
}

namespace
{
	// Cast's curve mode for a WraithAnim type. Translations of a relative (and delta: the delta tag is
	// one more relative bone in Cast) animation are offsets from the rest pose; its rotations and
	// scales are the bone's whole local value, as in SEAnim's relative type: absolute in Cast.
	const char* CastCurveMode(WraithAnimationType Type, bool Translation)
	{
		switch (Type)
		{
		case WraithAnimationType::Absolute: return "absolute";
		case WraithAnimationType::Additive: return "additive";
		default: return Translation ? "relative" : "absolute";
		}
	}

	// One curve per channel of a bone's keys (tx/ty/tz, sx/sy/sz: a float each; rq: the quaternion),
	// its key frames as narrow as its last frame allows
	template <typename TValue, size_t Channels>
	void AddCurves(CastNode& Anim, const std::string& Bone, const std::vector<WraithAnimFrame<TValue>>& Keys,
		const char* const (&Names)[Channels], const char* Mode)
	{
		if (Keys.empty())
			return;

		uint32_t LargestFrame = 0;
		for (auto& Key : Keys)
			LargestFrame = std::max(LargestFrame, Key.Frame);
		const auto FrameType = CastIntegerType(LargestFrame);

		for (size_t Channel = 0; Channel < Channels; Channel++)
		{
			auto Curve = Anim.AddNode(CastNodeId::Curve);
			Curve->SetProperty("nn", Bone);
			Curve->SetProperty("kp", Names[Channel]);
			Curve->SetProperty("m", Mode);
			auto Frames = Curve->AddProperty("kb", FrameType);
			auto Values = Curve->AddProperty("kv", std::is_same_v<TValue, Quaternion> ? CastPropertyId::Vector4 : CastPropertyId::Float);
			for (auto& Key : Keys)
			{
				if constexpr (std::is_same_v<TValue, Quaternion>)
					Values->Write(Key.Value);
				else
					Values->Write(Channel == 0 ? Key.Value.X : Channel == 1 ? Key.Value.Y : Key.Value.Z);
				CastWriteInteger(*Frames, FrameType, Key.Frame);
			}
		}
	}
}

void Cast::ExportCastAnim(const WraithAnim& Anim, const std::string& FileName, bool SupportsScale)
{
	BinaryWriter Writer;

	if (!Writer.Create(FileName))
		return;

	CastNode CastRoot;
	auto CastAnim = CastRoot.AddNode(CastNodeId::Animation);
	AddMetadata(CastRoot);

	CastAnim->SetProperty("fr", CastPropertyId::Float, Anim.FrameRate);
	CastAnim->SetProperty("lo", CastPropertyId::Byte, Anim.Looping);

	static const char* const Translation[] = { "tx", "ty", "tz" };
	static const char* const Rotation[] = { "rq" };
	static const char* const Scale[] = { "sx", "sy", "sz" };
	for (auto& [Bone, Keys] : Anim.AnimationPositionKeys)
		AddCurves(*CastAnim, Bone, Keys, Translation, CastCurveMode(Anim.AnimType, true));
	for (auto& [Bone, Keys] : Anim.AnimationRotationKeys)
		AddCurves(*CastAnim, Bone, Keys, Rotation, CastCurveMode(Anim.AnimType, false));
	for (auto& [Bone, Keys] : Anim.AnimationScaleKeys)
		AddCurves(*CastAnim, Bone, Keys, Scale, CastCurveMode(Anim.AnimType, false));

	for (auto& BoneModifier : Anim.AnimationBoneModifiers)
	{
		auto CastOverride = CastAnim->AddNode(CastNodeId::CurveModeOverride);
		CastOverride->SetProperty("nn", BoneModifier.first);
		CastOverride->SetProperty("ot", CastPropertyId::Byte, (uint8_t)1);
		CastOverride->SetProperty("m", CastCurveMode(BoneModifier.second, true));
	}

	for (auto& Note : Anim.AnimationNotetracks)
	{
		auto CastNote = CastAnim->AddNode(CastNodeId::NotificationTrack);
		CastNote->SetProperty("n", Note.first);
		auto KeyFrameBuffer = CastNote->AddProperty("kb", CastPropertyId::Integer32);
		for (auto& Key : Note.second)
			KeyFrameBuffer->Write(Key);
	}

	for (auto& Blend : Anim.AnimationBlendShapeWeightKeys)
	{
		static const char* const Weight[] = { "bs" };
		AddCurves(*CastAnim, Blend.first, Blend.second, Weight, "absolute");
	}

	WriteCastFile(Writer, CastRoot);
}

CastNode::CastNode() : Identifier(CastNodeId::Root), Hash(0)
{
}

CastNode::CastNode(const CastNodeId id) : Identifier(id), Hash(0)
{

}

CastNode::CastNode(const CastNodeId id, const uint64_t hash) : Identifier(id), Hash(hash)
{
}

const size_t CastNode::Size() const
{
	size_t result = 24;

	for (auto& prop : Properties)
	{
		result += 8;
		result += prop.first.size();
		result += prop.second->Buffer.size();
	}

	for (auto& child : Children)
	{
		result += child->Size();
	}

	return result;
}

CastNode* CastNode::AddNode(const CastNodeId id)
{
	Children.emplace_back(std::make_unique<CastNode>(id));
	return Children.back().get();
}

CastNode* CastNode::AddNode(const CastNodeId id, const uint64_t hash)
{
	Children.emplace_back(std::make_unique<CastNode>(id, hash));
	return Children.back().get();
}
