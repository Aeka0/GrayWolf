#pragma once

#include <string>

class WraithModel;

class WavefrontOBJ
{
public:
    static void ExportOBJ(const WraithModel& Model, const std::string& FileName);
};