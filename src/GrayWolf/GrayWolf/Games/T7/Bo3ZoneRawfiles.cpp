#include "stdafx.h"

#include "Bo3ZoneRawfiles.h"

#include "ExportNaming.h"
#include "OatBridge.h"       // ActsDecompileGsc
#include "FileSystems.h"

#include <filesystem>
#include <fstream>

namespace
{
    namespace fs = std::filesystem;

    // Forward slashes, no leading slash, no parent-directory escapes; BO3 rawfile names often
    // carry a real subdirectory (e.g. "scripts/zm/zm_bgb.gsc").
    bool SafeRelPath(std::string Name, std::string& OutRel)
    {
        for (auto& C : Name) if (C == '\\') C = '/';
        while (!Name.empty() && Name.front() == '/') Name.erase(Name.begin());
        if (Name.empty() || Name.find("..") != std::string::npos) return false;
        OutRel = Name;
        return true;
    }

    // \x80GSC\r\n. Anything else under the generic "rawfile" bucket (csv/json/ddl/text) is plain
    // content already.
    bool LooksLikeGsc(const uint8_t* Data, size_t Size)
    {
        return Size >= 6 && Data[0] == 0x80 && Data[1] == 'G' && Data[2] == 'S' && Data[3] == 'C';
    }
}

bool Bo3WriteRawfilePayload(const uint8_t* Data, size_t Size, const std::string& Name,
                            const std::string& OutDir, const std::string& ActsExe,
                            size_t& Decompiled, size_t& DecompileFailed, size_t& WriteFailed)
{
    std::string Rel;
    if (!SafeRelPath(Name, Rel)) return false;

    const bool IsGsc = LooksLikeGsc(Data, Size);
    const fs::path OutPath = fs::path(OutDir) / Rel;
    std::error_code MkEc;
    fs::create_directories(OutPath.parent_path(), MkEc);

    if (!IsGsc)
    {
        std::ofstream Out(OutPath, std::ios::binary);
        if (!Out) { WriteFailed++; return false; }
        Out.write((const char*)Data, (std::streamsize)Size);
        if (!Out) { WriteFailed++; return false; }
        return true;
    }

    // Compiled script: decompile via acts (OatBridge.h). acts needs the bytecode on disk first
    // and picks GSC/CSC from the file's own extension.
    char TempBase[MAX_PATH]{};
    char TempDir[MAX_PATH]{};
    GetTempPathA(MAX_PATH, TempDir);
    GetTempFileNameA(TempDir, "rgs", 0, TempBase);
    DeleteFileA(TempBase);

    const std::string Ext = FileSystems::GetExtension(Name);
    const std::string ScriptPath = std::string(TempBase) + (Ext.empty() ? ".gsc" : Ext);
    const std::string DecompileOutDir = std::string(TempBase) + "_out";

    {
        std::ofstream ScriptOut(ScriptPath, std::ios::binary);
        if (!ScriptOut) { WriteFailed++; return false; }
        ScriptOut.write((const char*)Data, (std::streamsize)Size);
    }

    std::string DecompiledSource, DecompileLog;
    const bool Ok = ActsDecompileGsc(ActsExe, ScriptPath, DecompileOutDir, DecompiledSource, DecompileLog);

    DeleteFileA(ScriptPath.c_str());
    std::error_code Ec;
    fs::remove_all(DecompileOutDir, Ec);

    if (!Ok || DecompiledSource.empty())
    {
        DecompileFailed++;
        // Still write the raw bytecode - a real result beats nothing, and the failure count
        // above keeps this from silently looking like a clean decompile.
        std::ofstream Out(OutPath, std::ios::binary);
        if (!Out) { WriteFailed++; return false; }
        Out.write((const char*)Data, (std::streamsize)Size);
        return true;
    }

    std::ofstream Out(OutPath, std::ios::binary);
    if (!Out) { WriteFailed++; return false; }
    Out.write(DecompiledSource.data(), (std::streamsize)DecompiledSource.size());
    if (!Out) { WriteFailed++; return false; }
    Decompiled++;
    return true;
}
