#pragma once

// (kept minimal on purpose - this header is included from the exporter hot path)

#include <cstdint>
#include <string>
#include <vector>

// Writes binary FBX 7400: Blender reads only binary 7100+, and 7400 is the last with 32-bit offsets.
// A tree of records, each stating its end offset: written to a buffer and patched, not streamed.
class FBXNode
{
public:

    explicit FBXNode(std::string Name);

    void AddInt32(int32_t Value);
    void AddInt64(int64_t Value);
    void AddDouble(double Value);
    void AddString(const std::string& Value);
    // Object names in FBX are "name\0\1Class", which is the one piece of the format that
    // looks like a mistake and is not
    void AddObjectName(const std::string& Name, const std::string& Class);

    // Arrays. These are the bulk of a model file, and are deflated when that is smaller.
    void AddDoubleArray(const std::vector<double>& Values);
    void AddInt32Array(const std::vector<int32_t>& Values);

    FBXNode& AddChild(const std::string& Name);

    // The top level of a file is a bare list rather than one record, so the writer needs
    // to walk the root's children itself
    const std::vector<FBXNode>& Children() const { return Nested; }

    // Writes this record and everything under it, patching in the end offset once it is
    // known
    void Write(std::vector<uint8_t>& Out) const;

private:

    std::string RecordName;
    std::vector<uint8_t> Properties;
    uint32_t PropertyCount = 0;
    std::vector<FBXNode> Nested;
};

// Everything needed to write one model, in the shape the writer wants rather than the
// shape the exporter happens to hold
struct FBXModelInput
{
    struct Vertex
    {
        double Position[3] = {};
        double Normal[3] = {};
        double UV[2] = {};
        // Bone index and weight pairs, already normalised by the caller
        std::vector<std::pair<int32_t, double>> Weights;
    };

    struct Mesh
    {
        std::string Name;
        int32_t MaterialIndex = -1;
        std::vector<Vertex> Vertices;
        // Three indices per face, into this mesh's own vertices
        std::vector<int32_t> Indices;
    };

    struct Bone
    {
        std::string Name;
        int32_t Parent = -1;
        double Position[3] = {};
        // Euler angles in degrees, which is what FBX stores
        double Rotation[3] = {};
        double Scale[3] = { 1.0, 1.0, 1.0 };
    };

    std::string Name;
    std::vector<Mesh> Meshes;
    std::vector<Bone> Bones;
    std::vector<std::string> Materials;
};

class FBXWriter
{
public:

    // Writes the model to a binary FBX file. Returns false if the file could not be
    // written; a model with no meshes is still a valid file.
    static bool Write(const std::string& FilePath, const FBXModelInput& Model);
};
