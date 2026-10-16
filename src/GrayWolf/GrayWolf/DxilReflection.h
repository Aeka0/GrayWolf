#pragma once

// A DXIL shader's reflection (a DXBC container holding DXIL, Shader Model 6), for the techsets kind
// (Techsets.h). The games' shaders are stripped (no STAT or RDAT part, no names): DXC's container
// reflection rebuilds the bindings, constant buffer sizes and signatures from the DXIL part; its
// disassembly gives the types' names (a cbuffer's struct, a resource's HLSL type) and the members'
// layout by the legacy cbuffer rules. Without dxcompiler.dll, the PSV0 part's bindings and the
// ISG1 / OSG1 signatures are read directly.

#include <nlohmann/json.hpp>

#include <unknwn.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace DxilReflection
{
    // A member of a constant buffer: by its struct type's name or by rank (names are stripped)
    struct Variable
    {
        std::string Name, Type;
        uint32_t Offset = 0, Size = 0;
    };

    struct Resource
    {
        int Class = 0;            // 0 SRV, 1 UAV, 2 CBV, 3 sampler (DXIL's resource classes)
        std::string Kind;         // texture, byteaddress, structured, cbuffer, sampler, uav_typed...
        std::string Dimension;    // 2d, cube, buffer... ("" unknown)
        std::string HlslType;     // Texture2D<float4>, ByteAddressBuffer, a cbuffer's struct name... ("" unknown)
        uint32_t Space = 0, Slot = 0;
        int32_t Count = 1;        // 0: an unbounded array (a bindless table)
        int RangeId = -1;         // its range id in the DXIL module (createHandle's)
        uint32_t Size = 0;        // a cbuffer's
        std::vector<Variable> Vars;
        bool LayoutChecked = false;   // a cbuffer whose members, laid out, fill its size
    };

    struct Parameter
    {
        std::string Semantic, Component;
        uint32_t Index = 0, Register = 0, SystemValue = 0;
        uint8_t Mask = 0, Used = 0;
    };

    // A load of a buffer at the shader's base plus a constant offset; TextureIndex when the value
    // loaded (a component of it, maybe offset, masked or through a waterfall loop) indexes an SRV
    // table, unbounded or an array (a bindless texture)
    struct BufferRead
    {
        uint32_t Offset = 0;
        bool TextureIndex = false;
    };

    struct Shader
    {
        bool Ok = false;
        std::string Status;        // what was read: "dxil", "stat", "psv0", or why nothing
        std::string Stage;         // vs, hs, ds, gs, ps, cs, lib...; the DXIL program kind
        std::string ShaderModel;   // "6_5"
        std::vector<std::string> Parts;
        std::vector<Parameter> Inputs, Outputs;
        std::vector<Resource> Resources;
        std::vector<BufferRead> Reads;
    };

    // Reflects a DXBC container; ReadsOf (space, register): the buffer whose constant-offset loads
    // Shader::Reads lists (a game's material buffer)
    Shader Reflect(const uint8_t* Data, size_t Size, std::optional<std::pair<uint32_t, uint32_t>> ReadsOf = std::nullopt);
    // The DXIL program kind of the container's DXIL part ("ps", "vs"...), "" when none
    std::string ProgramKind(const uint8_t* Data, size_t Size);
    // Where dxcompiler.dll was loaded from, "" when none loads (beside the exe, a Windows SDK's x64
    // bin, then the DLL search path)
    std::string Compiler();
    // A DXC object (CLSID_DxcLibrary, CLSID_DxcContainerReflection...) from that DLL, for a reader
    // reflecting shaders its own way; E_FAIL when none loads
    HRESULT CreateInstance(REFCLSID Clsid, REFIID Iid, void** Out);
    template <class T>
    HRESULT CreateInstance(REFCLSID Clsid, T** Out) { return CreateInstance(Clsid, __uuidof(T), (void**)Out); }

    // As shaders.json writes it: shader_model, reflection, parts, inputs, outputs, cbuffers,
    // resources, material_reads
    void ToJson(const Shader& S, nlohmann::ordered_json& Out);
}
