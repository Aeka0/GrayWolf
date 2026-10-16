#pragma once

#include <string>

class WraithModel;
class WraithAnim;

class GLTF
{
public:
    static void ExportGLTF(const WraithModel& Model, const std::string& FileName, bool SupportsScale = false, bool writeBinary = false);
    static void ExportGLTF(const WraithAnim& Model, const std::string& FileName, bool SupportsScale = false);
};