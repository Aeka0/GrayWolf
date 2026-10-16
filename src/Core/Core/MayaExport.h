#pragma once

#include <string>

class WraithModel;

class Maya
{
public:
    // Export a WraithModel to a Maya ASCII file (Bind included in the same folder)
    static void ExportMaya(const WraithModel& Model, const std::string& FileName);
};