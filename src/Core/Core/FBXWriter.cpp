#include "stdafx.h"

#include "FBXWriter.h"

#include <miniz/miniz.h>
#include "BinaryWriter.h"

#include <cstring>

namespace
{
    // Version 7400. The header is a fixed magic, two marker bytes, then the version.
    constexpr uint32_t FBXVersion = 7400;
    const char FBXMagic[] = "Kaydara FBX Binary  ";

    // A record with no properties and no children still costs this much: three offsets
    // and a zero length name
    constexpr size_t NullRecordSize = 13;

    template<typename T> void Append(std::vector<uint8_t>& Out, const T& Value)
    {
        const auto* Bytes = reinterpret_cast<const uint8_t*>(&Value);
        Out.insert(Out.end(), Bytes, Bytes + sizeof(T));
    }

    void AppendBytes(std::vector<uint8_t>& Out, const void* Data, size_t Length)
    {
        const auto* Bytes = reinterpret_cast<const uint8_t*>(Data);
        Out.insert(Out.end(), Bytes, Bytes + Length);
    }

    // Arrays may be stored raw or deflated (the record says which); deflating wins on the
    // large vertex arrays but loses on tiny ones, so the smaller of the two is written.
    template<typename T>
    void AppendArray(std::vector<uint8_t>& Out, char Type, const std::vector<T>& Values)
    {
        Out.push_back((uint8_t)Type);
        Append(Out, (uint32_t)Values.size());

        const auto Raw = Values.size() * sizeof(T);

        std::vector<uint8_t> Deflated(mz_compressBound((mz_ulong)Raw) + 16);
        auto DeflatedSize = (mz_ulong)Deflated.size();

        const auto Ok = Raw > 0 && mz_compress2(
            Deflated.data(), &DeflatedSize,
            (const unsigned char*)Values.data(), (mz_ulong)Raw,
            MZ_DEFAULT_COMPRESSION) == MZ_OK;

        if (Ok && DeflatedSize < Raw)
        {
            Append(Out, (uint32_t)1);            // encoding: deflate
            Append(Out, (uint32_t)DeflatedSize);
            AppendBytes(Out, Deflated.data(), DeflatedSize);
        }
        else
        {
            Append(Out, (uint32_t)0);            // encoding: none
            Append(Out, (uint32_t)Raw);
            AppendBytes(Out, Values.data(), Raw);
        }
    }
}

FBXNode::FBXNode(std::string Name)
    : RecordName(std::move(Name))
{
}

void FBXNode::AddInt32(int32_t Value)
{
    Properties.push_back('I');
    Append(Properties, Value);
    PropertyCount++;
}

void FBXNode::AddInt64(int64_t Value)
{
    Properties.push_back('L');
    Append(Properties, Value);
    PropertyCount++;
}

void FBXNode::AddDouble(double Value)
{
    Properties.push_back('D');
    Append(Properties, Value);
    PropertyCount++;
}

void FBXNode::AddString(const std::string& Value)
{
    Properties.push_back('S');
    Append(Properties, (uint32_t)Value.size());
    AppendBytes(Properties, Value.data(), Value.size());
    PropertyCount++;
}

void FBXNode::AddObjectName(const std::string& Name, const std::string& Class)
{
    // The separator really is a null byte followed by 0x01
    std::string Joined = Name;
    Joined.push_back('\0');
    Joined.push_back('\1');
    Joined += Class;

    AddString(Joined);
}

void FBXNode::AddDoubleArray(const std::vector<double>& Values)
{
    AppendArray(Properties, 'd', Values);
    PropertyCount++;
}

void FBXNode::AddInt32Array(const std::vector<int32_t>& Values)
{
    AppendArray(Properties, 'i', Values);
    PropertyCount++;
}

FBXNode& FBXNode::AddChild(const std::string& Name)
{
    Nested.emplace_back(Name);
    return Nested.back();
}

void FBXNode::Write(std::vector<uint8_t>& Out) const
{
    // The end offset is not known until everything below has been written, so leave room
    // and come back for it
    const auto OffsetPosition = Out.size();

    Append(Out, (uint32_t)0);
    Append(Out, (uint32_t)PropertyCount);
    Append(Out, (uint32_t)Properties.size());
    Out.push_back((uint8_t)RecordName.size());
    AppendBytes(Out, RecordName.data(), RecordName.size());
    AppendBytes(Out, Properties.data(), Properties.size());

    if (!Nested.empty())
    {
        for (const auto& Child : Nested)
        {
            Child.Write(Out);
        }

        // A list of children is closed by an empty record
        Out.insert(Out.end(), NullRecordSize, 0);
    }

    const auto End = (uint32_t)Out.size();
    std::memcpy(Out.data() + OffsetPosition, &End, sizeof(End));
}

namespace
{
    // Object ids only have to be unique and non zero. Counting up from something well
    // clear of the reserved low values keeps them readable in a hex dump.
    int64_t NextId(int64_t& Counter)
    {
        return ++Counter;
    }

    void WriteHeader(FBXNode& Root)
    {
        auto& Extension = Root.AddChild("FBXHeaderExtension");
        Extension.AddChild("FBXHeaderVersion").AddInt32(1003);
        Extension.AddChild("FBXVersion").AddInt32((int32_t)FBXVersion);

        auto& Creator = Root.AddChild("Creator");
        Creator.AddString("Greyhound");

        // Importers read unit scale and axis system from here - get it wrong and models
        // arrive 100x too large or on their side. Call of Duty is Z-up, in inches (2.54cm).
        auto& Settings = Root.AddChild("GlobalSettings");
        Settings.AddChild("Version").AddInt32(1000);
        auto& Props = Settings.AddChild("Properties70");

        const auto Int = [&Props](const char* Name, int32_t Value)
        {
            auto& P = Props.AddChild("P");
            P.AddString(Name); P.AddString("int"); P.AddString("Integer"); P.AddString("");
            P.AddInt32(Value);
        };

        const auto Number = [&Props](const char* Name, double Value)
        {
            auto& P = Props.AddChild("P");
            P.AddString(Name); P.AddString("double"); P.AddString("Number"); P.AddString("");
            P.AddDouble(Value);
        };

        Int("UpAxis", 2);            // Z
        Int("UpAxisSign", 1);
        Int("FrontAxis", 1);         // Y
        Int("FrontAxisSign", -1);
        Int("CoordAxis", 0);         // X
        Int("CoordAxisSign", 1);
        Number("UnitScaleFactor", 2.54);
    }

    // Every object class has to be declared before it is used
    void WriteDefinitions(FBXNode& Root, const FBXModelInput& Model)
    {
        auto& Definitions = Root.AddChild("Definitions");
        Definitions.AddChild("Version").AddInt32(100);

        const auto Meshes = (int32_t)Model.Meshes.size();
        const auto Bones = (int32_t)Model.Bones.size();
        const auto Materials = (int32_t)Model.Materials.size();

        // Geometries, models for each mesh and each bone, deformers, materials
        Definitions.AddChild("Count").AddInt32(Meshes * 3 + Bones + Materials + 1);

        const auto Declare = [&Definitions](const char* Type, int32_t Count)
        {
            if (Count <= 0)
            {
                return;
            }

            auto& Object = Definitions.AddChild("ObjectType");
            Object.AddString(Type);
            Object.AddChild("Count").AddInt32(Count);
        };

        Declare("Geometry", Meshes);
        Declare("Model", Meshes + Bones);
        Declare("Material", Materials);
        Declare("Deformer", Meshes * 2);
        Declare("Pose", 1);
    }

    void WriteMeshGeometry(FBXNode& Objects, const FBXModelInput::Mesh& Mesh, int64_t Id)
    {
        auto& Geometry = Objects.AddChild("Geometry");
        Geometry.AddInt64(Id);
        Geometry.AddObjectName(Mesh.Name, "Geometry");
        Geometry.AddString("Mesh");

        std::vector<double> Positions;
        std::vector<double> Normals;
        std::vector<double> UVs;
        Positions.reserve(Mesh.Vertices.size() * 3);
        Normals.reserve(Mesh.Vertices.size() * 3);
        UVs.reserve(Mesh.Vertices.size() * 2);

        for (const auto& Vertex : Mesh.Vertices)
        {
            Positions.push_back(Vertex.Position[0]);
            Positions.push_back(Vertex.Position[1]);
            Positions.push_back(Vertex.Position[2]);

            Normals.push_back(Vertex.Normal[0]);
            Normals.push_back(Vertex.Normal[1]);
            Normals.push_back(Vertex.Normal[2]);

            UVs.push_back(Vertex.UV[0]);
            // FBX puts the texture origin at the bottom left, the game at the top left
            UVs.push_back(1.0 - Vertex.UV[1]);
        }

        Geometry.AddChild("Vertices").AddDoubleArray(Positions);

        // The last index of each polygon is stored negated and off by one, which is how
        // the format marks where one face ends and the next begins
        std::vector<int32_t> Indices;
        Indices.reserve(Mesh.Indices.size());

        for (size_t i = 0; i + 2 < Mesh.Indices.size(); i += 3)
        {
            Indices.push_back(Mesh.Indices[i]);
            Indices.push_back(Mesh.Indices[i + 1]);
            Indices.push_back(~Mesh.Indices[i + 2]);
        }

        Geometry.AddChild("PolygonVertexIndex").AddInt32Array(Indices);

        auto& NormalLayer = Geometry.AddChild("LayerElementNormal");
        NormalLayer.AddInt32(0);
        NormalLayer.AddChild("Version").AddInt32(101);
        NormalLayer.AddChild("Name").AddString("");
        NormalLayer.AddChild("MappingInformationType").AddString("ByVertice");
        NormalLayer.AddChild("ReferenceInformationType").AddString("Direct");
        NormalLayer.AddChild("Normals").AddDoubleArray(Normals);

        auto& UVLayer = Geometry.AddChild("LayerElementUV");
        UVLayer.AddInt32(0);
        UVLayer.AddChild("Version").AddInt32(101);
        UVLayer.AddChild("Name").AddString("map1");
        UVLayer.AddChild("MappingInformationType").AddString("ByVertice");
        UVLayer.AddChild("ReferenceInformationType").AddString("Direct");
        UVLayer.AddChild("UV").AddDoubleArray(UVs);

        auto& MaterialLayer = Geometry.AddChild("LayerElementMaterial");
        MaterialLayer.AddInt32(0);
        MaterialLayer.AddChild("Version").AddInt32(101);
        MaterialLayer.AddChild("Name").AddString("");
        MaterialLayer.AddChild("MappingInformationType").AddString("AllSame");
        MaterialLayer.AddChild("ReferenceInformationType").AddString("IndexToDirect");
        MaterialLayer.AddChild("Materials").AddInt32Array({ 0 });

        auto& Layer = Geometry.AddChild("Layer");
        Layer.AddInt32(0);
        Layer.AddChild("Version").AddInt32(100);

        const auto Element = [&Layer](const char* Type)
        {
            auto& Entry = Layer.AddChild("LayerElement");
            Entry.AddChild("Type").AddString(Type);
            Entry.AddChild("TypedIndex").AddInt32(0);
        };

        Element("LayerElementNormal");
        Element("LayerElementUV");
        Element("LayerElementMaterial");
    }

    void WriteModelNode(FBXNode& Objects, const std::string& Name, const char* Kind, int64_t Id,
        const double Position[3], const double Rotation[3], const double Scale[3])
    {
        auto& Node = Objects.AddChild("Model");
        Node.AddInt64(Id);
        Node.AddObjectName(Name, "Model");
        Node.AddString(Kind);

        Node.AddChild("Version").AddInt32(232);

        auto& Props = Node.AddChild("Properties70");

        const auto Vector = [&Props](const char* PropName, const char* Type, const double Value[3])
        {
            auto& P = Props.AddChild("P");
            P.AddString(PropName); P.AddString(Type); P.AddString(""); P.AddString("A");
            P.AddDouble(Value[0]); P.AddDouble(Value[1]); P.AddDouble(Value[2]);
        };

        if (Position != nullptr) Vector("Lcl Translation", "Lcl Translation", Position);
        if (Rotation != nullptr) Vector("Lcl Rotation", "Lcl Rotation", Rotation);
        if (Scale != nullptr)    Vector("Lcl Scaling", "Lcl Scaling", Scale);
    }
}

bool FBXWriter::Write(const std::string& FilePath, const FBXModelInput& Model)
{
    FBXNode Root("");

    WriteHeader(Root);
    WriteDefinitions(Root, Model);

    int64_t Counter = 1000000;

    auto& Objects = Root.AddChild("Objects");
    auto& Connections = Root.AddChild("Connections");

    const auto Connect = [&Connections](int64_t Child, int64_t Parent)
    {
        auto& C = Connections.AddChild("C");
        C.AddString("OO");
        C.AddInt64(Child);
        C.AddInt64(Parent);
    };

    // Bones first, so the meshes can bind to them
    std::vector<int64_t> BoneIds;
    BoneIds.reserve(Model.Bones.size());

    for (const auto& Bone : Model.Bones)
    {
        const auto Id = NextId(Counter);
        BoneIds.push_back(Id);
        WriteModelNode(Objects, Bone.Name, "LimbNode", Id, Bone.Position, Bone.Rotation, Bone.Scale);
    }

    for (size_t i = 0; i < Model.Bones.size(); i++)
    {
        // A bone with no parent hangs off the scene root, which is always id zero
        const auto Parent = Model.Bones[i].Parent;
        Connect(BoneIds[i], (Parent >= 0 && (size_t)Parent < BoneIds.size()) ? BoneIds[Parent] : 0);
    }

    std::vector<int64_t> MaterialIds;
    MaterialIds.reserve(Model.Materials.size());

    for (const auto& Name : Model.Materials)
    {
        const auto Id = NextId(Counter);
        MaterialIds.push_back(Id);

        auto& Material = Objects.AddChild("Material");
        Material.AddInt64(Id);
        Material.AddObjectName(Name, "Material");
        Material.AddString("");
        Material.AddChild("Version").AddInt32(102);
        Material.AddChild("ShadingModel").AddString("lambert");
        Material.AddChild("Properties70");
    }

    for (const auto& Mesh : Model.Meshes)
    {
        const auto GeometryId = NextId(Counter);
        const auto ModelId = NextId(Counter);

        WriteMeshGeometry(Objects, Mesh, GeometryId);

        static const double Origin[3] = { 0.0, 0.0, 0.0 };
        static const double Unit[3] = { 1.0, 1.0, 1.0 };
        WriteModelNode(Objects, Mesh.Name, "Mesh", ModelId, Origin, Origin, Unit);

        Connect(GeometryId, ModelId);
        Connect(ModelId, 0);

        if (Mesh.MaterialIndex >= 0 && (size_t)Mesh.MaterialIndex < MaterialIds.size())
        {
            Connect(MaterialIds[Mesh.MaterialIndex], ModelId);
        }

        if (Model.Bones.empty())
        {
            continue;
        }

        // Skinning. One deformer for the mesh, then one sub deformer per bone that
        // actually has weight on it - writing empty clusters is legal but wasteful.
        const auto SkinId = NextId(Counter);

        auto& Skin = Objects.AddChild("Deformer");
        Skin.AddInt64(SkinId);
        Skin.AddObjectName(Mesh.Name + "_skin", "Deformer");
        Skin.AddString("Skin");
        Skin.AddChild("Version").AddInt32(101);
        Skin.AddChild("Link_DeformAcuracy").AddDouble(50.0);

        Connect(SkinId, GeometryId);

        std::vector<std::vector<int32_t>> ClusterIndices(Model.Bones.size());
        std::vector<std::vector<double>> ClusterWeights(Model.Bones.size());

        for (size_t v = 0; v < Mesh.Vertices.size(); v++)
        {
            for (const auto& Weight : Mesh.Vertices[v].Weights)
            {
                if (Weight.first < 0 || (size_t)Weight.first >= Model.Bones.size() || Weight.second <= 0.0)
                {
                    continue;
                }

                ClusterIndices[Weight.first].push_back((int32_t)v);
                ClusterWeights[Weight.first].push_back(Weight.second);
            }
        }

        for (size_t b = 0; b < Model.Bones.size(); b++)
        {
            if (ClusterIndices[b].empty())
            {
                continue;
            }

            const auto ClusterId = NextId(Counter);

            auto& Cluster = Objects.AddChild("Deformer");
            Cluster.AddInt64(ClusterId);
            Cluster.AddObjectName(Model.Bones[b].Name + "_cluster", "SubDeformer");
            Cluster.AddString("Cluster");
            Cluster.AddChild("Version").AddInt32(100);
            Cluster.AddChild("Indexes").AddInt32Array(ClusterIndices[b]);
            Cluster.AddChild("Weights").AddDoubleArray(ClusterWeights[b]);

            Connect(ClusterId, SkinId);
            Connect(BoneIds[b], ClusterId);
        }
    }

    std::vector<uint8_t> Out;
    Out.reserve(1024 * 1024);

    // FBXMagic (20 chars) + null terminator + 2 marker bytes + version = 23 bytes before
    // the first record. Dropping the null shifts every offset after it by one.
    AppendBytes(Out, FBXMagic, sizeof(FBXMagic));
    Out.push_back(0x1A);
    Out.push_back(0x00);
    Append(Out, FBXVersion);

    for (const auto& Child : Root.Children())
    {
        Child.Write(Out);
    }

    // The top level list is closed the same way as any other
    Out.insert(Out.end(), NullRecordSize, 0);

    auto Writer = BinaryWriter();

    if (!Writer.Create(FilePath))
    {
        return false;
    }

    Writer.Write((const int8_t*)Out.data(), (uint32_t)Out.size());
    return true;
}
