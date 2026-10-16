#pragma once

#include <string>

class WraithModel;

class ValveSMD
{
public:
    static void ExportSMD(const WraithModel& Model, const std::string& FileName);
};