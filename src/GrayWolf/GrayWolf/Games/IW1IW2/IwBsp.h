#pragma once

// The levels of CoD1 (maps/<name>.bsp, "IBSP" version 59, 33 lumps) and CoD2 (maps/<name>.d3dbsp,
// "IBSP" version 4, 39 lumps): a lump table (length, offset), the lumps one after another, each
// padded to 4 bytes. What a map export needs: the materials, the triangle soups (the drawn
// surfaces, the curved ones already tessellated), their vertices and indices, and the entity
// string.

#include "MapLive.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace IW12
{
    struct BspFile
    {
        struct Material
        {
            std::string Name;
            uint32_t SurfaceFlags = 0, ContentFlags = 0;
        };
        // a run of triangles of one material over a run of vertices; indices count from FirstVertex
        struct Soup
        {
            uint16_t Material = 0, DrawOrder = 0;
            uint32_t FirstVertex = 0;
            uint16_t VertexCount = 0, IndexCount = 0;
            uint32_t FirstIndex = 0;
        };
        int32_t Version = 0;
        size_t LumpCount = 0;
        std::vector<Material> Materials;
        std::vector<Soup> Soups;
        std::vector<Maps::Vertex> Vertices;
        std::vector<uint16_t> Indices;
        std::string Entities;

        // The collision (Quake 3's): planes (normal, dist); brush sides, the first six of a brush
        // its bounds (a float: min x, max x, min y, max y, min z, max z), the others a plane
        // number, each with a material; brushes (a side count and the material giving the brush
        // its content flags), their sides one brush after another; brush models (their bounds,
        // then first and count of their soups, of their collision surfaces, of their brushes;
        // model 0 is the world).
        struct Plane
        {
            float Normal[3] = { 0, 0, 0 };
            float Dist = 0;
        };
        struct BrushSide
        {
            uint32_t PlaneOrBound = 0;   // a plane number, or a float (the first six)
            uint32_t Material = 0;
        };
        struct Brush
        {
            uint16_t Sides = 0, Material = 0;
        };
        struct Model
        {
            float Mins[3] = { 0, 0, 0 }, Maxs[3] = { 0, 0, 0 };
            uint32_t FirstSoup = 0, Soups = 0, FirstCollision = 0, Collisions = 0, FirstBrush = 0, Brushes = 0;
        };
        std::vector<Plane> Planes;
        std::vector<BrushSide> BrushSides;
        std::vector<Brush> Brushes;
        std::vector<Model> Models;

        // CoD1's curved and terrain collision (lump 24, 16 bytes): a material, a type, then for a
        // terrain (type 1) its vertex and index counts, first vertex and first index (indices
        // from its first vertex), for a patch (type 0) the width and height of its grid of
        // Bezier control points (odd), a number not read and its first vertex. A model's
        // collision surfaces are a run of PatchRefs (lump 23: numbers of these).
        struct Patch
        {
            uint16_t Material = 0, Type = 0, Width = 0, Height = 0;   // terrain: vertex count, index count
            uint32_t Other = 0, FirstVertex = 0;                      // terrain: first vertex, first index
        };
        std::vector<Patch> Patches;
        std::vector<uint32_t> PatchRefs;
        std::vector<float> CollisionVertices;   // xyz (CoD1 lump 25, CoD2 lump 29 after a stamp)
        std::vector<uint16_t> CollisionIndices; // CoD1 lump 26

        // CoD2's collision triangles (lump 31, 72 bytes): plane (normal, dist), the barycentric
        // planes s and t (s . p - s.w is 0 at the first vertex, 1 at the second), three vertex
        // numbers (0xFFFFFFFF: a vertex the lump of vertices lacks, where s and t put it) and
        // three edge numbers; partitions (lump 33, 12 bytes: u16, u8 triangle count, u8 border
        // count, first triangle, first border); the AABB tree (lump 34, 32 bytes: centre, half
        // size, u16 material, u16 child count, first child or (no child) its partition). A
        // model's collision surfaces are a run of tree nodes.
        struct Triangle
        {
            float Plane[4] = { 0, 0, 0, 0 }, S[4] = { 0, 0, 0, 0 }, T[4] = { 0, 0, 0, 0 };
            uint32_t Vertices[3] = { 0, 0, 0 };
        };
        struct Partition
        {
            uint8_t Triangles = 0;
            uint32_t FirstTriangle = 0;
        };
        struct AabbNode
        {
            uint16_t Material = 0, Children = 0;
            uint32_t First = 0;
        };
        std::vector<Triangle> Triangles;
        std::vector<Partition> Partitions;
        std::vector<AabbNode> AabbTree;
    };

    // False with Error unless: the lumps follow one another from the table's end, each padded to 4
    // bytes, to the end of the file; the lumps read divide into whole records; every soup's
    // material, vertices and indices exist, each index within its soup's vertices; the entity
    // string ends with its lump's last byte; the brushes' sides fill the sides' lump, each side's
    // plane and material and each brush's material exist; the models' brushes and collision
    // surfaces exist; every collision surface's material, vertices and indices exist (CoD1), every
    // triangle's vertex (or 0xFFFFFFFF), partition's triangles and tree node's children or
    // partition and material exist (CoD2).
    bool ParseBsp(const uint8_t* Data, size_t Size, BspFile& Out, std::string& Error);

    // The level's collision for the map writers: the world's brushes and each brush model's (the
    // sides' material names as the Radiant of CoD2 on names its tools), the curved and terrain
    // collision (CoD1's patches tessellated) when its material collides: the world's as
    // triangles, a brush model's as brushes of it (a prism per triangle, Maps::TrianglePrism).
    // Counters: what was left out (collision_*).
    void BspCollision(const BspFile& Bsp, Maps::Collision& Out, std::map<std::string, size_t>& Counters);

    // The entity string as key/value maps, in order (MapLive.h, shared by every map reader).
    using Entity = Maps::Entity;
    using Maps::ParseEntities;
}
