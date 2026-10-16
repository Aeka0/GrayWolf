#pragma once

#include <string>
#include <cstdint>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "WraithAnim.h"

class WraithMaterial;
class WraithModel;

class CastNode;

// A material as Cast's texture slots ("albedo", "normal", "gloss", "roughness", "metal", "specular",
// "ao", "emissive", "extra0"...), each a file path relative to the .cast file: the caller turns its
// own material description into these (GrayWolf: the game's MaterialAdapter).
struct CastMaterialSlots
{
	std::string Name;
	std::vector<std::pair<std::string, std::string>> Files;
};

class Cast
{
public:
	// Export a WraithModel to a Cast file. Slots, when given, replace the diffuse/normal/specular
	// triple of the WraithMaterial of the same name.
	static void ExportCastModel(const WraithModel& Model, const std::string& FileName, bool SupportsScale = false,
		const std::vector<CastMaterialSlots>* Slots = nullptr);
	static void ExportCastAnim(const WraithAnim& Anim, const std::string& FileName, bool SupportsScale = false);

	// For other writers (the map scene): a Metadata node under Root (the software, Z up as CoD is;
	// SceneRoot, relative to the file, is where the importers resolve its instances' files), a
	// Material node with its slots, and the file itself.
	static void AddMetadata(CastNode& Root, const std::string& SceneRoot = "");
	static CastNode* AddMaterial(CastNode& Model, const CastMaterialSlots& Material);
	static bool WriteFile(const CastNode& Root, const std::string& FileName);
};

enum class CastNodeId : uint32_t
{
	Root              = 0x746F6F72,
	Model             = 0x6C646F6D,
	Mesh              = 0x6873656D,
	BlendShape        = 0x68736C62,
	Skeleton          = 0x6C656B73,
	Bone              = 0x656E6F62,
	Animation         = 0x6D696E61,
	Curve             = 0x76727563,
	NotificationTrack = 0x6669746E,
	CurveModeOverride = 0x564F4D43,
	Material          = 0x6C74616D,
	File              = 0x656C6966,
	Color             = 0x726C6F63,
	Instance          = 0x74736E69,
	Metadata          = 0x6174656D,
};

enum class CastPropertyId : uint16_t
{
	Byte      = 'b',
	Short     = 'h',
	Integer32 = 'i',
	Integer64 = 'l',
	Float     = 'f',
	Double    = 'd',
	String    = 's',
	Vector2   = 'v2',
	Vector3   = 'v3',
	Vector4   = 'v4'
};

class CastProperty
{
public:
	CastPropertyId Identifier;
	size_t Elements;
	std::vector<char> Buffer;

	CastProperty() : Identifier(CastPropertyId::Byte), Elements(0) {}

	template <class T>
	void Write(const T data)
	{
		Elements++;
		auto asChar = (const unsigned char*)&data;
		for (size_t i = 0; i < sizeof(T); i++)
		{
			Buffer.push_back(asChar[i]);
		}
	}

	void Write(const char* data, const size_t len)
	{
		Elements++;
		for (size_t i = 0; i < len; i++)
		{
			Buffer.push_back(data[i]);
		}
	}
};

class CastNode
{
public:
	CastNodeId Identifier;
	uint64_t Hash;
	std::map<std::string, std::unique_ptr<CastProperty>> Properties;
	std::vector<std::unique_ptr<CastNode>> Children;

	CastNode();
	CastNode(const CastNodeId id);
	CastNode(const CastNodeId id, const uint64_t hash);

	CastProperty* AddProperty(const std::string& propName, const CastPropertyId id)
	{
		auto prop = std::make_unique<CastProperty>();
		auto result = prop.get();
		prop->Identifier = id;
		Properties[propName] = std::move(prop);
		return result;
	}

	CastProperty* AddProperty(const std::string& propName, const CastPropertyId id, const size_t capacity)
	{
		auto prop = std::make_unique<CastProperty>();
		auto result = prop.get();
		prop->Identifier = id;
		prop->Buffer.reserve(capacity);
		Properties[propName] = std::move(prop);
		return result;
	}

	void SetProperty(const std::string& propName, const std::string& value)
	{
		auto prop = std::make_unique<CastProperty>();
		prop->Identifier = CastPropertyId::String;
		prop->Write(value.c_str(), value.length() + 1);
		Properties[propName] = std::move(prop);
	}
	
	template <class T>
	void SetProperty(const std::string& propName, const CastPropertyId id, const T value)
	{
		auto prop = std::make_unique<CastProperty>();
		prop->Identifier = id;
		prop->Write<T>(value);
		Properties[propName] = std::move(prop);
	}

	const size_t Size() const;

	CastNode* AddNode(const CastNodeId id);
	CastNode* AddNode(const CastNodeId id, const uint64_t hash);
};

// The narrowest integer type holding every value up to Max (Byte, Short or Integer32), and a value
// written to a property at that type: face indices, bone indices, key frames alike.
CastPropertyId CastIntegerType(uint64_t Max);
void CastWriteInteger(CastProperty& Prop, CastPropertyId Type, uint32_t Value);

// One Cast mesh node, fed vertex by vertex then face by face, whoever the caller (a model, a map
// surface): the property layout (positions, normals, one colour layer, UV layers, weights), the
// index width and the winding (reversed, as the importers expect CoD's) are Cast's, in one place.
class CastMeshBuilder
{
public:
	// Bones: the bone count when MaxInfluence > 0 (the weight index width)
	CastMeshBuilder(CastNode& Model, uint64_t Hash, size_t Vertices, uint32_t UVLayers,
		uint32_t MaxInfluence = 0, size_t Bones = 0);
	// UVs: UVLayers entries
	void Vertex(const Vector3& Position, const Vector3& Normal, uint32_t Color, const Vector2* UVs);
	// MaxInfluence calls per vertex, after its Vertex (0, 0.0f pads a vertex with fewer)
	void Weight(uint32_t Bone, float Value);
	// A triangle as the game winds it; a degenerate one is left out
	void Face(uint32_t A, uint32_t B, uint32_t C);
	void Material(uint64_t Hash);

	CastNode* Node() const { return Mesh; }
	// the width of a vertex index in this mesh (a blend shape's vertex indices use it too)
	CastPropertyId IndexType() const { return Indices; }

private:
	CastNode* Mesh;
	CastPropertyId Indices, BoneIndices = CastPropertyId::Byte;
	CastProperty *Positions, *Normals, *Colors, *Faces, *WeightBones = nullptr, *WeightValues = nullptr;
	std::vector<CastProperty*> UVs;
};
