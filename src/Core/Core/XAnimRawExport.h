#pragma once

#include <string>
#include <cstdint>

class WraithAnim;

enum class XAnimRawVersion : uint16_t
{
    WorldAtWar = 17,
    BlackOps = 19
};

class XAnimRaw
{
public:
    static void ExportXAnimRaw(const WraithAnim& Animation, const std::string& FileName, XAnimRawVersion Version = XAnimRawVersion::WorldAtWar);
};