#pragma once

#include <cstdint>
#include <string>


// One rawfile/scriptparsetree payload at a zonekit-walked position (Games/T7/DiskGame.cpp's
// ExportRawfiles): detects a compiled GSC/CSC script and decompiles it via acts, otherwise writes the bytes as-is. True
// when a file was written (Decompiled also incremented for a real decompile); false leaves every
// counter but WriteFailed untouched (a name that can't be made into a safe relative path).
bool Bo3WriteRawfilePayload(const uint8_t* Data, size_t Size, const std::string& Name,
                            const std::string& OutDir, const std::string& ActsExe,
                            size_t& Decompiled, size_t& DecompileFailed, size_t& WriteFailed);
